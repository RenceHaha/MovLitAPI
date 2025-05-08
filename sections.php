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
                addSection($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can add sections']);
            }
            break;
        case 'edit':
            if ($access == "admin") {
                editSection($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can edit sections']);
            }
            break;
        case 'delete':
            if ($access == "admin") {
                deleteSection($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can delete sections']);
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

function addSection($data) {
    global $conn;

    if (!isset($data['section_name'])) {
        echo json_encode(['success' => false, 'message' => 'Section name missing']);
        return;
    }

    $section_name = $data['section_name'];

    $sql = "INSERT INTO sections_tbl (section_name) VALUES (?)";
    $statement = $conn->prepare($sql);
    $statement->bind_param("s", $section_name);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Section added successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to add section']);
    }
    $statement->close();
}

function editSection($data) {
    global $conn;

    if (!isset($data['section_id'])) {
        echo json_encode(['success' => false, 'message' => 'Section ID missing']);
        return;
    }

    if (!isset($data['section_name'])) {
        echo json_encode(['success' => false, 'message' => 'Section name missing']);
        return;
    }

    $section_id = $data['section_id'];
    $section_name = $data['section_name'];

    $sql = "UPDATE sections_tbl SET section_name = ? WHERE section_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("si", $section_name, $section_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Section updated successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to update section']);
    }
    $statement->close();
}

function deleteSection($data) {
    global $conn;

    if (!isset($data['section_id'])) {
        echo json_encode(['success' => false, 'message' => 'Section ID missing']);
        return;
    }

    $section_id = $data['section_id'];

    $sql = "DELETE FROM sections_tbl WHERE section_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $section_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'Section deleted successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to delete section']);
    }
    $statement->close();
}
?>