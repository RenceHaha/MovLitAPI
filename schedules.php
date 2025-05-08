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
    if($access == "Unauthorized") {
        echo json_encode(['success' => false,'message' => 'User not authorized']);
        return;
    }
    switch ($data['action']) {
        case 'add':
            addSchedule($data);
            break;
        case 'edit':
            editSchedule($data);
            break;
        case 'delete':
            deleteSchedule($data);
            break;
        case 'get':
            getSchedule($data, $access);
            break;
        case 'getSpecific':
            getSpecific($data, $access);
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

    $sql = "SELECT * FROM account_tbl WHERE account_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);
    $statement->execute();
    $result = $statement->get_result();

    if ($result->num_rows > 0) {
        $access_level = $result->fetch_assoc()['access'];
        if ($access_level == "admin" || $access_level == "professor" || $access_level == "staff") {
            echo json_encode(['success' => true, 'message' => 'User verified']);
            return $access_level;
        } else {
            echo json_encode(['success' => false, 'message' => 'User not authorized']);
            return "Unauthorized";
        }
    } else {
        echo json_encode(['success' => false, 'message' => 'User not found']);
        return "User not found";
    }
}

// Function to add a new user
function addSchedule($data) {
    global $conn;

    if (!isset($data['room_id'])) {
        echo json_encode(['success' => false, 'message' => 'Room ID missing']);
        return;
    }

    if (!isset($data['section_id'])) {
        echo json_encode(['success' => false, 'message' => 'Section ID missing']);
        return;
    }

    if (!isset($data['account_id'])) {
        echo json_encode(['success' => false, 'message' => 'Account ID missing']);
        return;
    }

    if (!isset($data['start']) || !isset($data['end'])) {
        echo json_encode(['success' => false, 'message' => 'Start or end time missing']);
        return;
    }

    $room_id = $data['room_id'];
    $section_id = $data['section_id'];
    $account_id = $data['account_id'];
    $frequency = $data['frequency'];
    $start = $data['start'];
    $end = $data['end'];
    $one_time = isset($data['one_time']) ? $data['one_time'] : null;
    $start_date = isset($data['start_date']) ? $data['start_date'] : null;
    $end_date = isset($data['end_date']) ? $data['end_date'] : null;
    $specific_date = isset($data['specific_date']) ? $data['specific_date'] : null;

    try {
        $conn->begin_transaction();
        $sql = "INSERT INTO schedule_tbl (room_id, section_id, account_id, start, end";
        $params = [$room_id, $section_id, $account_id, $start, $end];
        $types = "iiiss"; // Assuming room_id, section_id, account_id are integers, start and end are strings

        if ($frequency != null) {
            foreach ($frequency as $day) {
                $day = strtolower($day);
                $sql .= ", $day";
                $params[] = 1; // Assuming the days are stored as integers (1 for true)
                $types .= "i";
            }
        }
        if ($start_date) {
            $sql .= ", start_date";
            $params[] = $start_date;
            $types .= "s";
        }
        if ($end_date) {
            $sql .= ", end_date";
            $params[] = $end_date;
            $types .= "s";
        }
        if ($specific_date) {
            $sql .= ", specific_date";
            $params[] = $specific_date;
            $types .= "s";
        }
        if ($one_time !== null) {
            $sql .= ", one_time";
            $params[] = $one_time;
            $types .= "i";
        }

        $sql .= ") VALUES (?,?,?,?,?";
        if ($frequency != null) {
            foreach ($frequency as $day) {
                $sql .= ", ?";
            }
        }
        if ($start_date) {
            $sql .= ", ?";
        }
        if ($end_date) {
            $sql .= ", ?";
        }
        if ($specific_date) {
            $sql .= ", ?";
        }
        if ($one_time !== null) {
            $sql .= ", ?";
        }
        $sql .= ")";

        $statement = $conn->prepare($sql);
        $statement->bind_param($types, ...$params);

        if ($statement->execute()) {
            echo json_encode(['success' => true, 'message' => 'Schedule added successfully']);
            $conn->commit();
        } else {
            echo json_encode(['success' => false, 'message' => 'Failed to insert schedule']);
        }
        $statement->close();
    } catch (Exception $e) {
        echo json_encode(['success' => false, 'message' => $e->getMessage()]);
        $conn->rollback();
    }
}

function editSchedule($data) {
    global $conn;

    if (!isset($data['schedule_id'])) {
        echo json_encode(['success' => false, 'message' => 'Schedule ID missing']);
        return;
    }

    $schedule_id = $data['schedule_id'];
    $fields = [];
    $params = [];
    $types = "";

    if (isset($data['room_id'])) {
        $fields[] = "room_id = ?";
        $params[] = $data['room_id'];
        $types .= "i";
    }
    if (isset($data['section_id'])) {
        $fields[] = "section_id = ?";
        $params[] = $data['section_id'];
        $types .= "i";
    }
    if (isset($data['account_id'])) {
        $fields[] = "account_id = ?";
        $params[] = $data['account_id'];
        $types .= "i";
    }
    if (isset($data['start'])) {
        $fields[] = "start = ?";
        $params[] = $data['start'];
        $types .= "s";
    }
    if (isset($data['end'])) {
        $fields[] = "end = ?";
        $params[] = $data['end'];
        $types .= "s";
    }

    if (!empty($fields)) {
        $sql = "UPDATE schedule_tbl SET " . implode(", ", $fields) . " WHERE schedule_id = ?";
        $params[] = $schedule_id;
        $types .= "i";

        $statement = $conn->prepare($sql);
        $statement->bind_param($types, ...$params);

        if ($statement->execute()) {
            echo json_encode(['success' => true, 'message' => 'Schedule updated successfully']);
        } else {
            echo json_encode(['success' => false, 'message' => 'Failed to update schedule']);
        }
        $statement->close();
    } else {
        echo json_encode(['success' => false, 'message' => 'No fields to update']);
    }
}

function deleteSchedule($data) {
    global $conn;

    if (!isset($data['schedule_id'])) {
        echo json_encode(['success' => false, 'message' => 'Schedule ID missing']);
        return;
    }

    $schedule_id = $data['schedule_id'];

    $sql = "DELETE FROM schedule_tbl WHERE schedule_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $schedule_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Schedule deleted successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to delete schedule']);
    }
    $statement->close();
}

function getSchedule($data, $access) {
    global $conn;

    $sql = "";
    if($access == "staff") {
        $sql = "SELECT * FROM schedule_tbl";
    }else if($access == "professor") {
        $sql = "SELECT * FROM schedule_tbl WHERE account_id =?";
    }else if($access == "admin") {
        $sql = "SELECT * FROM schedule_tbl";
    }
    $statement = $conn->prepare($sql);
    if($access == "professor") {    
        $statement->bind_param("s", $data['account_id']);
    }
    $statement->execute();
    $result = $statement->get_result();

    if ($row = $result->fetch_assoc()) {
        echo json_encode(['success' => true, 'schedule' => $row]);
    } else {
        echo json_encode(['success' => false, 'message' => 'Schedule empty']);
    }
    $statement->close();
}

function getSpecific($data, $access) {
    global $conn;

    if (!isset($data['schedule_id'])) {
        echo json_encode(['success' => false, 'message' => 'Schedule ID missing']);
        return;
    }

    $sql = "";
    if ($access == "staff" || $access == "admin") {
        $sql = "SELECT * FROM schedule_tbl WHERE schedule_id = ?";
        $statement = $conn->prepare($sql);
        $statement->bind_param("i", $data['schedule_id']);
    } else if ($access == "professor") {
        $sql = "SELECT * FROM schedule_tbl WHERE account_id = ? AND schedule_id = ?";
        $statement = $conn->prepare($sql);
        $statement->bind_param("ii", $data['account_id'], $data['schedule_id']);
    }

    $statement->execute();
    $result = $statement->get_result();

    if ($row = $result->fetch_assoc()) {
        echo json_encode(['success' => true, 'schedule' => $row]);
    } else {
        echo json_encode(['success' => false, 'message' => 'Schedule not found']);
    }
    $statement->close();
}


?>
