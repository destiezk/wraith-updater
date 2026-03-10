<?php

ini_set('display_errors', 1);
error_reporting(E_ALL);

$debugFile = __DIR__ . '/token_debug.log';
file_put_contents($debugFile, "START\n", FILE_APPEND);

$tokenDir = __DIR__ . '/tokens';
if (!is_dir($tokenDir)) {
    mkdir($tokenDir, 0777, true);
}
file_put_contents($debugFile, "Tokens dir OK\n", FILE_APPEND);

file_put_contents($debugFile, print_r($_SERVER, true), FILE_APPEND);

$headerValid = isset($_SERVER['HTTP_X_WRAITH_CLIENT']) && $_SERVER['HTTP_X_WRAITH_CLIENT'] == '1';
if (!$headerValid) {
    file_put_contents($debugFile, "CUSTOM HEADER MISSING OR INVALID\n", FILE_APPEND);
    http_response_code(403);
    echo "Forbidden";
    exit;
}

if (function_exists('openssl_random_pseudo_bytes')) {
    $tokenBytes = openssl_random_pseudo_bytes(16);
    file_put_contents($debugFile, "Used openssl_random_pseudo_bytes()\n", FILE_APPEND);
} else {
    file_put_contents($debugFile, "NO CRYPTO RANDOM AVAILABLE, USING mt_rand\n", FILE_APPEND);
    $tokenBytes = '';
    for ($i = 0; $i < 16; $i++) {
        $tokenBytes .= chr(mt_rand(0, 255));
    }
}

$token = bin2hex($tokenBytes);

$ttlSeconds = 600;
$expires = time() + $ttlSeconds;
$ip = isset($_SERVER['REMOTE_ADDR']) ? $_SERVER['REMOTE_ADDR'] : '';

$dataArray = array(
    'exp' => $expires,
    'ip'  => $ip
);

$dataJson = json_encode($dataArray);

$tokenFile = $tokenDir . '/' . $token . '.json';
if (!file_put_contents($tokenFile, $dataJson)) {
    file_put_contents($debugFile, "FAILED TO WRITE TOKEN FILE\n", FILE_APPEND);
    http_response_code(500);
    exit;
}

file_put_contents($debugFile, "TOKEN CREATED: $token\n", FILE_APPEND);

header('Content-Type: text/plain');
echo $token;
?>
