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
                addUser($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can add users']);
            }
            break;
        case 'delete':
            if ($access == "admin") {
                deleteUser($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'Only admin can delete users']);
            }
            break;
        case 'edit_profile':
            if ($access == "admin" || $access == "professor" || $access == "staff") {
                editProfile($data);
            } else {
                echo json_encode(['success' => false, 'message' => 'You do not have permission to edit profiles']);
            }
            break;
        case 'fetch_profile':
            getUserProfile($data['account_id']);
            break;
        case 'change_password':
            changePassword($data);
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

function addUser($data) {
    global $conn;

    if (!isset($data['username'], $data['password'], $data['email'], $data['access_level'])) {
        echo json_encode(['success' => false, 'message' => 'Missing parameters']);
        return;
    }

    $username = $data['username'];
    $password = password_hash($data['password'], PASSWORD_BCRYPT);
    $email = $data['email'];
    $access = $data['access_level'];

    try {
        $conn->begin_transaction();
        $sql = "INSERT INTO account_tbl (username, password, email, access) VALUES (?, ?, ?, ?)";
        $statement = $conn->prepare($sql);
        $statement->bind_param("ssss", $username, $password, $email, $access);

        if ($statement->execute()) {
            echo json_encode(['success' => true, 'message' => 'User added successfully']);
            $conn->commit();
        } else {
            echo json_encode(['success' => false, 'message' => 'Failed to insert user']);
        }
        $statement->close();
    } catch (Exception $e) {
        echo json_encode(['success' => false, 'message' => $e->getMessage()]);
        $conn->rollback();
    }
}

function deleteUser($data) {
    global $conn;

    if (!isset($data['account_id'])) {
        echo json_encode(['success' => false, 'message' => 'Account ID missing']);
        return;
    }

    $account_id = $data['account_id'];

    $sql = "DELETE FROM account_tbl WHERE account_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);

    if ($statement->execute()) {
        echo json_encode(['success' => true, 'message' => 'User deleted successfully']);
    } else {
        echo json_encode(['success' => false, 'message' => 'Failed to delete user']);
    }
    $statement->close();
}

function editProfile($data) {
    global $conn;

    $imagePath = null;

    if (!isset($data['account_id'])) {
        echo json_encode(['success' => false, 'message' => 'Account ID is required']);
        return;
    }

    if (isset($data['image'])) {
        $base64String = $data['image'];
        if (preg_match('/^data:image\/(\w+);base64,/', $base64String, $type)) {
            $base64String = substr($base64String, strpos($base64String, ',') + 1);
            $imageType = strtolower($type[1]);

            $base64String = base64_decode($base64String);
            if ($base64String === false) {
                echo json_encode(['success' => false, 'message' => 'Base64 decode failed.']);
                return;
            }

            $targetDir = "images/";
            $fileName = uniqid() . '.' . $imageType;
            $targetFile = $targetDir . $fileName;

            if (file_put_contents($targetFile, $base64String) !== false) {
                $imagePath = $targetFile;
            } else {
                echo json_encode(['success' => false, 'message' => 'Failed to save the image.']);
                return;
            }
        } else {
            echo json_encode(['success' => false, 'message' => 'Invalid base64 image format.']);
            return;
        }
    }

    $account_id = $data['account_id'];

    $check_sql = "SELECT * FROM profile_tbl WHERE account_id = ?";
    $check_statement = $conn->prepare($check_sql);
    $check_statement->bind_param("i", $account_id);
    $check_statement->execute();
    $check_result = $check_statement->get_result();
    $user_exists = $check_result->num_rows > 0;
    $check_statement->close();

    try {
        $conn->begin_transaction();

        if (isset($data['username']) || isset($data['email'])) {
            $account_fields = [];
            $account_params = [];
            $account_types = "";

            if (isset($data['username'])) {
                $account_fields[] = "username = ?";
                $account_params[] = $data['username'];
                $account_types .= "s";
            }

            if (isset($data['email'])) {
                $account_fields[] = "email = ?";
                $account_params[] = $data['email'];
                $account_types .= "s";
            }

            if (!empty($account_fields)) {
                $sql = "UPDATE account_tbl SET " . implode(", ", $account_fields) . " WHERE account_id = ?";
                $account_params[] = $account_id;
                $account_types .= "i";

                $statement = $conn->prepare($sql);
                $statement->bind_param($account_types, ...$account_params);
                $statement->execute();
                $statement->close();
            }
        }

        if (isset($data['first_name']) || isset($data['middle_name']) || isset($data['last_name']) || isset($data['suffix']) || isset($data['contact'])) {
            $user_fields = [];
            $user_params = [];
            $user_types = "";

            if (isset($data['first_name'])) {
                $user_fields[] = "first_name = ?";
                $user_params[] = $data['first_name'];
                $user_types .= "s";
            }

            if (isset($data['middle_name'])) {
                $user_fields[] = "middle_name = ?";
                $user_params[] = $data['middle_name'];
                $user_types .= "s";
            }

            if (isset($data['last_name'])) {
                $user_fields[] = "last_name = ?";
                $user_params[] = $data['last_name'];
                $user_types .= "s";
            }

            if (isset($data['suffix'])) {
                $user_fields[] = "suffix = ?";
                $user_params[] = $data['suffix'];
                $user_types .= "s";
            }

            if (isset($data['contact'])) {
                $user_fields[] = "contact = ?";
                $user_params[] = $data['contact'];
                $user_types .= "s";
            }

            if (isset($imagePath)) {
                $user_fields[] = "image_path = ?";
                $user_params[] = $imagePath;
                $user_types .= "s";
            }

            if (!empty($user_fields)) {
                if (!$user_exists) {
                    $sql = "INSERT INTO profile_tbl (account_id, " . implode(", ", $user_fields) . ") VALUES (?, " . str_repeat("?, ", count($user_fields) - 1) . "?)";
                    $user_params = array_merge([$account_id], $user_params);
                    $user_types = "i" . $user_types;
                } else {
                    $sql = "UPDATE profile_tbl SET " . implode(", ", $user_fields) . " WHERE account_id = ?";
                    $user_params[] = $account_id;
                    $user_types .= "i";
                }

                $statement = $conn->prepare($sql);
                $statement->bind_param($user_types, ...$user_params);
                $statement->execute();
                $statement->close();
            }
        }

        $conn->commit();
        echo json_encode(['success' => true, 'message' => 'Profile updated successfully']);
    } catch (Exception $e) {
        $conn->rollback();
        echo json_encode(['success' => false, 'message' => 'Error: ' . $e->getMessage()]);
    }
}

function getUserProfile($account_id) {
    global $conn;

    $sql = "SELECT a.username, a.email, a.created_at, 
                   u.first_name, u.middle_name, u.last_name, u.suffix, u.image_path
            FROM account_tbl a
            LEFT JOIN profile_tbl u ON a.account_id = u.account_id
            WHERE a.account_id = ?";

    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);
    $statement->execute();
    $result = $statement->get_result();

    if ($row = $result->fetch_assoc()) {
        echo json_encode([
            'success' => true,
            'user' => $row
        ]);
    } else {
        echo json_encode(['success' => false, 'message' => 'User not found']);
    }

    $statement->close();
}

function changePassword($data) {
    global $conn;

    if (!isset($data['account_id'], $data['current_password'], $data['new_password'])) {
        echo json_encode(['success' => false, 'message' => 'Missing parameters']);
        return;
    }

    $account_id = $data['account_id'];
    $current_password = $data['current_password'];
    $new_password = $data['new_password'];

    $sql = "SELECT password FROM account_tbl WHERE account_id = ?";
    $statement = $conn->prepare($sql);
    $statement->bind_param("i", $account_id);
    $statement->execute();
    $statement->bind_result($hashedPassword);

    if ($statement->fetch() && password_verify($current_password, $hashedPassword)) {
        $statement->close();

        $new_hashed_password = password_hash($new_password, PASSWORD_BCRYPT);

        $sql = "UPDATE account_tbl SET password = ? WHERE account_id = ?";
        $update_statement = $conn->prepare($sql);
        $update_statement->bind_param("si", $new_hashed_password, $account_id);

        if ($update_statement->execute()) {
            echo json_encode(['success' => true, 'message' => 'Password changed successfully']);
        } else {
            echo json_encode(['success' => false, 'message' => 'Failed to update password']);
        }
        $update_statement->close();
    } else {
        echo json_encode(['success' => false, 'message' => 'Incorrect current password']);
        $statement->close();
    }
}
?>
