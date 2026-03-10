<?php
// PHP 5.6 compatible wraith_update.php - FIXED

ini_set('display_errors', 1);
error_reporting(E_ALL);

$tokenDir = __DIR__ . '/tokens';
$acFile = __DIR__ . '/154287251jf98jf98jfaskjfakof/WRAITH-AC.asi';
$runtimeFile = __DIR__ . '/154287251jf98jf98jfaskjfakof/onnxruntime.dll';

// -- 1) Check custom header --
$headerValid = isset($_SERVER['HTTP_X_WRAITH_CLIENT']) && $_SERVER['HTTP_X_WRAITH_CLIENT'] == '1';
if (!$headerValid) {
    http_response_code(403);
    echo "Forbidden";
    exit;
}

// -- 2) Token validation --
if (!isset($_GET['token'])) {
    http_response_code(400);
    echo "Missing token";
    exit;
}

$token = $_GET['token'];

if (!preg_match('/^[0-9a-f]{32}$/', $token)) {
    http_response_code(400);
    echo "Invalid token format";
    exit;
}

$tokenPath = $tokenDir . '/' . $token . '.json';
if (!file_exists($tokenPath)) {
    http_response_code(403);
    echo "Invalid or already used token";
    exit;
}

// Read token
$dataJson = file_get_contents($tokenPath);
$data = json_decode($dataJson, true);

if (!is_array($data) || !isset($data['exp']) || !isset($data['ip'])) {
    http_response_code(403);
    echo "Token data corrupted";
    exit;
}

// Check expiration ONLY
if ($data['exp'] < time()) {
    http_response_code(403);
    echo "Token expired";
    unlink($tokenPath);
    exit;
}

// Check IP
$clientIp = isset($_SERVER['REMOTE_ADDR']) ? $_SERVER['REMOTE_ADDR'] : '';
if ($clientIp !== '' && $data['ip'] !== '' && $data['ip'] !== $clientIp) {
    http_response_code(403);
    echo "IP mismatch";
    exit;
}

// -- 3) Check files exist --
if (!file_exists($acFile)) {
    http_response_code(500);
    echo "AC file missing on server";
    exit;
}

if (!file_exists($runtimeFile)) {
    http_response_code(500);
    echo "Runtime file missing on server";
    exit;
}

// -- 4) Determine which file to send --
$fileType = isset($_GET['file']) ? $_GET['file'] : 'asi';
$fileToSend = '';
$filename = '';

if ($fileType === 'dll') {
    $fileToSend = $runtimeFile;
    $filename = 'onnxruntime.dll';
} else {
    $fileToSend = $acFile;
    $filename = 'WRAITH-AC.asi';
}

// -- 5) Send file --
header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="' . $filename . '"');
header('Content-Length: ' . filesize($fileToSend));
header('Cache-Control: no-cache, no-store, must-revalidate');
header('Pragma: no-cache');
header('Expires: 0');

readfile($fileToSend);
exit;
?>
