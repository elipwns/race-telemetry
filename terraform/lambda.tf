# Lambda Functions
# terraform apply auto-zips from lambda/ subdirectories

data "archive_file" "telemetry_ingest" {
  type        = "zip"
  source_dir  = "${path.module}/../lambda/telemetry-ingest"
  output_path = "${path.module}/telemetry-ingest.zip"
}

data "archive_file" "weather_ingest" {
  type        = "zip"
  source_dir  = "${path.module}/../lambda/weather-ingest"
  output_path = "${path.module}/weather-ingest.zip"
}

data "archive_file" "websocket_connect" {
  type        = "zip"
  source_dir  = "${path.module}/../lambda/websocket-connect"
  output_path = "${path.module}/websocket-connect.zip"
}

data "archive_file" "websocket_disconnect" {
  type        = "zip"
  source_dir  = "${path.module}/../lambda/websocket-disconnect"
  output_path = "${path.module}/websocket-disconnect.zip"
}

data "archive_file" "websocket_broadcast" {
  type        = "zip"
  source_dir  = "${path.module}/../lambda/websocket-broadcast"
  output_path = "${path.module}/websocket-broadcast.zip"
}

resource "aws_lambda_function" "telemetry_ingest" {
  function_name    = "telemetry-ingest"
  role             = aws_iam_role.telemetry_ingest.arn
  runtime          = "python3.12"
  handler          = "lambda_function.lambda_handler"
  filename         = data.archive_file.telemetry_ingest.output_path
  source_code_hash = data.archive_file.telemetry_ingest.output_base64sha256
}

resource "aws_lambda_function" "weather_ingest" {
  function_name    = "weather-ingest"
  role             = aws_iam_role.weather_ingest.arn
  runtime          = "python3.12"
  handler          = "lambda_function.lambda_handler"
  filename         = data.archive_file.weather_ingest.output_path
  source_code_hash = data.archive_file.weather_ingest.output_base64sha256
}

resource "aws_lambda_function" "websocket_connect" {
  function_name    = "rt-websocket-connect"
  role             = aws_iam_role.websocket_connect.arn
  runtime          = "python3.12"
  handler          = "lambda_function.lambda_handler"
  filename         = data.archive_file.websocket_connect.output_path
  source_code_hash = data.archive_file.websocket_connect.output_base64sha256
}

resource "aws_lambda_function" "websocket_disconnect" {
  function_name    = "rt-websocket-disconnect"
  role             = aws_iam_role.websocket_disconnect.arn
  runtime          = "python3.12"
  handler          = "lambda_function.lambda_handler"
  filename         = data.archive_file.websocket_disconnect.output_path
  source_code_hash = data.archive_file.websocket_disconnect.output_base64sha256
}

resource "aws_lambda_function" "websocket_broadcast" {
  function_name    = "rt-websocket-broadcast"
  role             = aws_iam_role.websocket_broadcast.arn
  runtime          = "python3.12"
  handler          = "lambda_function.lambda_handler"
  filename         = data.archive_file.websocket_broadcast.output_path
  source_code_hash = data.archive_file.websocket_broadcast.output_base64sha256
}
