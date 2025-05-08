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
        case 'add':
            if ($access == "admin") {
                addRoom($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can add rooms']);
            }
            break;
        case 'edit':
            if ($access == "admin") {
                editRoom($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can edit rooms']);
            }
            break;
        case 'delete':
            if ($access == "admin") {
                deleteRoom($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can delete rooms']);
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

function addRoom($data) {
    global $conn;

    if (!isset($data['room_number'])) {
        echo json_encode(['success' => false, 'message' => 'Room number missing']);
        return;
    }

    $room_number = $data['room_number'];

    $sql = "INSERT INTO room_tbl (room_number) VALUES (?)";
    $statement = $conn->prepare($sql);
    $statement->bind_param("s", $room_number);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Room added successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to add room']);
    }
    $statement->close();
}

function editRoom($data) {
    global $conn;

    if (!isset($data['room_id'])) {
        echo json_encode(['success' => false, 'message' => 'Room ID missing']);
        return;
    }

    if (!isset($data['room_number'])) {
        echo json_encode(['success' => false, 'message' => 'Room number missing']);
        return;
    }

    $room_id = $data['room_id'];
    $room_number = $data['room_number'];

    $sql = "UPDATE room_tbl SET room_number = ? WHERE room_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("si", $room_number, $room_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Room updated successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to update room']);
    }
    $statement->close();
}

function deleteRoom($data) {
    global $conn;

    if (!isset($data['room_id'])) {
        echo json_encode(['success' => false, 'message' => 'Room ID missing']);
        return;
    }

    $room_id = $data['room_id'];

    $sql = "DELETE FROM room_tbl WHERE room_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $room_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Room deleted successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to delete room']);
    }
    $statement->close();
}
?>