<?php
require_once('dbcon.php');
header('Content-Type: application/json');

// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Get the request method
$method = $_SERVER['REQUEST_METHOD'];

// Read JSON input
$json = file_get_contents('php://input');
$data = json_decode($json, true);

// Log the incoming request for debugging
file_put_contents('request_log.txt', date('Y-m-d H:i:s') . " - Request: " . $json . PHP_EOL, FILE_APPEND);

if ($method === 'POST') {
    // Check if this is a device control request
    if (isset($data['room_id']) && isset($data['light_state']) && isset($data['account_id'])) {
        handleRoomLighting($data);
    } else {
        echo json_encode(['success' => false, 'message' => 'Missing required parameters']);
    }
} else {
    echo json_encode(['success' => false, 'message' => 'Invalid request method']);
}

function handleRoomLighting($data) {
    global $conn;
    
    try {
        $room_id = $data['room_id'];
        $light_state = $data['light_state'];
        $account_id = $data['account_id'];
        
        // Validate account exists
        $sql = "SELECT * FROM account_tbl WHERE account_id = ?";
        $stmt = $conn->prepare($sql);
        $stmt->bind_param("i", $account_id);
        $stmt->execute();
        $result = $stmt->get_result();
        
        if ($result->num_rows == 0) {
            echo json_encode(['success' => false, 'message' => 'Invalid account']);
            return;
        }
        
        // Validate room exists
        $sql = "SELECT * FROM room_tbl WHERE room_id = ?";
        $stmt = $conn->prepare($sql);
        $stmt->bind_param("i", $room_id);
        $stmt->execute();
        $result = $stmt->get_result();
        
        if ($result->num_rows == 0) {
            echo json_encode(['success' => false, 'message' => 'Invalid room']);
            return;
        }
        
        // Log the lighting control action
        $sql = "INSERT INTO room_lighting_log (room_id, account_id, light_state, timestamp) 
                VALUES (?, ?, ?, NOW())";
        $stmt = $conn->prepare($sql);
        $stmt->bind_param("iii", $room_id, $account_id, $light_state);
        
        if ($stmt->execute()) {
            // Update the current state in room_tbl if you have such a column
            $sql = "UPDATE room_tbl SET light_state = ? WHERE room_id = ?";
            $stmt = $conn->prepare($sql);
            $stmt->bind_param("ii", $light_state, $room_id);
            $stmt->execute();
            
            echo json_encode([
                'success' => true, 
                'message' => 'Room lighting updated successfully',
                'data' => [
                    'room_id' => $room_id,
                    'light_state' => $light_state,
                    'timestamp' => date('Y-m-d H:i:s')
                ]
            ]);
        } else {
            echo json_encode(['success' => false, 'message' => 'Failed to update room lighting: ' . $conn->error]);
        }
    } catch (Exception $e) {
        // Catch any exceptions and return a valid JSON response
        echo json_encode(['success' => false, 'message' => 'Error: ' . $e->getMessage()]);
    }
}

// Make sure we always output something, even if there's an error
if (ob_get_length() === 0) {
    echo json_encode(['success' => false, 'message' => 'No response generated']);
}
?>