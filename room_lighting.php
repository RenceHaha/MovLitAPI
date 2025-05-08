<?php
require_once('dbcon.php');
header('Content-Type: application/json');

// Get the request method
$method = $_SERVER['REQUEST_METHOD'];

// Read JSON input
$json = file_get_contents('php://input');
$data = json_decode($json, true);

if ($method === 'POST' && isset($data['action'])) {
    $access = verifyUser($data);
    if ($access == "Unauthorized") {
        echo json_encode(['success' => false, 'message' => 'User not authorized']);
        return;
    }
    switch ($data['action']) {
        case 'override_light':
            overrideLight($data, $access);
            break;
        case 'reset_override':
            resetOverride($data);
            break;
        case 'device_control':
            if ($access == "device") {
                deviceControl($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only devices can perform this action']);
            }
            break;
        default:
            echo json_encode(['success' => false, 'message' => 'Invalid action']);
    }
} else {
    echo json_encode(['success' => false, 'message' => 'Invalid request']);
}

function verifyUser($data) {
    global $conn;

    if (!isset($data['account_id'])) {
        echo json_encode(['success' => false, 'message' => 'Account ID missing']);
        return "Account ID missing";
    }

    $account_id = $data['account_id'];

    $sql = "SELECT access FROM account_tbl WHERE account_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);
    $statement->execute();
    $result = $statement->get_result();

    if ($result->num_rows > 0) {
        $access_level = $result->fetch_assoc()['access'];
        return $access_level;
    } else {
        echo json_encode(['success' => false, 'message' => 'User not found']);
        return "Unauthorized";
    }
}

function overrideLight($data, $access) {
    global $conn;

    if (!isset($data['room_id'], $data['light_state'])) {
        echo json_encode(['success' => false, 'message' => 'Missing parameters']);
        return;
    }

    $room_id = $data['room_id'];
    $light_state = $data['light_state'];
    $account_id = $data['account_id'];

    // Check the latest log for this room
    $sql = "SELECT account_id, set_to_state, log_date FROM lighting_logs WHERE room_id = ? ORDER BY log_date DESC LIMIT 1";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $room_id);
    $statement->execute();
    $result = $statement->get_result();

    if ($result->num_rows > 0) {
        $log = $result->fetch_assoc();
        $last_modifier_access = getAccessLevel($log['account_id']);
        $last_log_time = strtotime($log['log_date']);
        $current_time = time();

        // Check if the last modification was recent
        if ($current_time - $last_log_time < 60) { // Assuming 60 seconds as a threshold
            if ($last_modifier_access == "admin" && ($access == "device" || $access == "staff")) {
                echo json_encode(['success' => false, 'message' => 'Modification not allowed due to recent admin change']);
                return;
            }
            if ($last_modifier_access == "staff" && $access != "device") {
                echo json_encode(['success' => false, 'message' => 'Modification not allowed due to recent staff change']);
                return;
            }
        }
    }

    // Update the light state
    $sql = "UPDATE room_lighting SET light_state = ?, set_to_state = ? WHERE room_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("iii", $light_state, $light_state, $room_id);

    if ($statement->execute()) {
        logLightChange($room_id, $account_id, $light_state);
        echo json_encode(['success' => true, 'message' => 'Light state overridden successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to override light state']);
    }
    $statement->close();
}

function resetOverride($data) {
    global $conn;

    if (!isset($data['room_id'])) {
        echo json_encode(['success' => false, 'message' => 'Room ID missing']);
        return;
    }

    $room_id = $data['room_id'];

    $sql = "UPDATE room_lighting SET set_to_state = 0 WHERE room_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $room_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Override reset successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to reset override']);
    }
    $statement->close();
}

function logLightChange($room_id, $account_id, $light_state) {
    global $conn;

    $sql = "INSERT INTO lighting_logs (room_id, account_id, set_to_state) VALUES (?, ?, ?)";
    $statement = $conn->prepare($sql);
    $statement->bind_param("iii", $room_id, $account_id, $light_state);
    $statement->execute();
    $statement->close();
}

function getAccessLevel($account_id) {
    global $conn;

    $sql = "SELECT access FROM account_tbl WHERE account_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);
    $statement->execute();
    $result = $statement->get_result();

    if ($result->num_rows > 0) {
        return $result->fetch_assoc()['access'];
    }
    return null;
}

function deviceControl($data) {
    global $conn;

    if (!isset($data['room_id'], $data['light_state'])) {
        echo json_encode(['success' => false, 'message' => 'Missing parameters']);
        return;
    }

    $room_id = $data['room_id'];
    $light_state = $data['light_state'];

    $sql = "UPDATE room_lighting SET light_state = ?, set_to_state = ? WHERE room_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("iii", $light_state, $light_state, $room_id);

    if ($statement->execute()) {
        logLightChange($room_id, $data['account_id'], $light_state);
        echo json_encode(['success' => true, 'message' => 'Device control executed successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to execute device control']);
    }
    $statement->close();
}
?>