# IAM Roles and Policies for Lambda functions

locals {
  lambda_assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action    = "sts:AssumeRole"
      Effect    = "Allow"
      Principal = { Service = "lambda.amazonaws.com" }
    }]
  })
}

# telemetry-ingest
resource "aws_iam_role" "telemetry_ingest" {
  name               = "telemetry-ingest-role"
  path               = "/service-role/"
  assume_role_policy = local.lambda_assume_role_policy
}

resource "aws_iam_role_policy" "telemetry_ingest" {
  name = "telemetry-ingest-policy"
  role = aws_iam_role.telemetry_ingest.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["dynamodb:PutItem"]
        Resource = aws_dynamodb_table.telemetry_runs.arn
      },
      {
        Effect   = "Allow"
        Action   = ["sns:Publish"]
        Resource = aws_sns_topic.telemetry_events.arn
      },
      {
        Effect   = "Allow"
        Action   = ["logs:CreateLogGroup", "logs:CreateLogStream", "logs:PutLogEvents"]
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# weather-ingest
resource "aws_iam_role" "weather_ingest" {
  name               = "weather-ingest-role"
  path               = "/service-role/"
  assume_role_policy = local.lambda_assume_role_policy
}

resource "aws_iam_role_policy" "weather_ingest" {
  name = "weather-ingest-policy"
  role = aws_iam_role.weather_ingest.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["dynamodb:PutItem"]
        Resource = aws_dynamodb_table.weather_readings.arn
      },
      {
        Effect   = "Allow"
        Action   = ["logs:CreateLogGroup", "logs:CreateLogStream", "logs:PutLogEvents"]
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# websocket-connect
resource "aws_iam_role" "websocket_connect" {
  name               = "websocket-connect-role"
  path               = "/service-role/"
  assume_role_policy = local.lambda_assume_role_policy
}

resource "aws_iam_role_policy" "websocket_connect" {
  name = "websocket-connect-policy"
  role = aws_iam_role.websocket_connect.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["dynamodb:PutItem"]
        Resource = aws_dynamodb_table.websocket_connections.arn
      },
      {
        Effect   = "Allow"
        Action   = ["logs:CreateLogGroup", "logs:CreateLogStream", "logs:PutLogEvents"]
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# websocket-disconnect
resource "aws_iam_role" "websocket_disconnect" {
  name               = "websocket-disconnect-role"
  path               = "/service-role/"
  assume_role_policy = local.lambda_assume_role_policy
}

resource "aws_iam_role_policy" "websocket_disconnect" {
  name = "websocket-disconnect-policy"
  role = aws_iam_role.websocket_disconnect.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["dynamodb:DeleteItem"]
        Resource = aws_dynamodb_table.websocket_connections.arn
      },
      {
        Effect   = "Allow"
        Action   = ["logs:CreateLogGroup", "logs:CreateLogStream", "logs:PutLogEvents"]
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# websocket-broadcast
resource "aws_iam_role" "websocket_broadcast" {
  name               = "websocket-broadcast-role"
  path               = "/service-role/"
  assume_role_policy = local.lambda_assume_role_policy
}

resource "aws_iam_role_policy" "websocket_broadcast" {
  name = "websocket-broadcast-policy"
  role = aws_iam_role.websocket_broadcast.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["dynamodb:Scan", "dynamodb:DeleteItem"]
        Resource = aws_dynamodb_table.websocket_connections.arn
      },
      {
        Effect   = "Allow"
        Action   = ["execute-api:ManageConnections"]
        Resource = "${aws_apigatewayv2_api.websocket.execution_arn}/*"
      },
      {
        Effect   = "Allow"
        Action   = ["logs:CreateLogGroup", "logs:CreateLogStream", "logs:PutLogEvents"]
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# Read-only role for data scientist
resource "aws_iam_role" "data_analyst" {
  name               = "race-telemetry-data-analyst"
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action    = "sts:AssumeRole"
      Effect    = "Allow"
      Principal = { AWS = "arn:aws:iam::${data.aws_caller_identity.current.account_id}:root" }
    }]
  })
}

resource "aws_iam_role_policy" "data_analyst" {
  name = "data-analyst-policy"
  role = aws_iam_role.data_analyst.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = ["dynamodb:Query", "dynamodb:Scan", "dynamodb:GetItem", "dynamodb:DescribeTable"]
        Resource = [
          aws_dynamodb_table.telemetry_runs.arn,
          aws_dynamodb_table.weather_readings.arn,
        ]
      }
    ]
  })
}

data "aws_caller_identity" "current" {}
