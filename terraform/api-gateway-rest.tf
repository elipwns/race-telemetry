# API Gateway REST API

resource "aws_api_gateway_rest_api" "telemetry_api" {
  name = "telemetry-api"
}

# /telemetry resource
resource "aws_api_gateway_resource" "telemetry" {
  rest_api_id = aws_api_gateway_rest_api.telemetry_api.id
  parent_id   = aws_api_gateway_rest_api.telemetry_api.root_resource_id
  path_part   = "telemetry"
}

resource "aws_api_gateway_method" "telemetry_post" {
  rest_api_id   = aws_api_gateway_rest_api.telemetry_api.id
  resource_id   = aws_api_gateway_resource.telemetry.id
  http_method   = "POST"
  authorization = "NONE"
}

resource "aws_api_gateway_integration" "telemetry_post" {
  rest_api_id             = aws_api_gateway_rest_api.telemetry_api.id
  resource_id             = aws_api_gateway_resource.telemetry.id
  http_method             = aws_api_gateway_method.telemetry_post.http_method
  integration_http_method = "POST"
  type                    = "AWS_PROXY"
  uri                     = aws_lambda_function.telemetry_ingest.invoke_arn
}

# /weather resource
resource "aws_api_gateway_resource" "weather" {
  rest_api_id = aws_api_gateway_rest_api.telemetry_api.id
  parent_id   = aws_api_gateway_rest_api.telemetry_api.root_resource_id
  path_part   = "weather"
}

resource "aws_api_gateway_method" "weather_post" {
  rest_api_id   = aws_api_gateway_rest_api.telemetry_api.id
  resource_id   = aws_api_gateway_resource.weather.id
  http_method   = "POST"
  authorization = "NONE"
}

resource "aws_api_gateway_integration" "weather_post" {
  rest_api_id             = aws_api_gateway_rest_api.telemetry_api.id
  resource_id             = aws_api_gateway_resource.weather.id
  http_method             = aws_api_gateway_method.weather_post.http_method
  integration_http_method = "POST"
  type                    = "AWS_PROXY"
  uri                     = aws_lambda_function.weather_ingest.invoke_arn
}

# Deployment
resource "aws_api_gateway_deployment" "telemetry_api" {
  rest_api_id = aws_api_gateway_rest_api.telemetry_api.id

  depends_on = [
    aws_api_gateway_integration.telemetry_post,
    aws_api_gateway_integration.weather_post,
  ]

  lifecycle {
    create_before_destroy = true
  }
}

resource "aws_api_gateway_stage" "prd" {
  rest_api_id   = aws_api_gateway_rest_api.telemetry_api.id
  deployment_id = aws_api_gateway_deployment.telemetry_api.id
  stage_name    = "prd"
}

# Lambda permissions
resource "aws_lambda_permission" "api_gateway_telemetry" {
  statement_id  = "AllowAPIGatewayTelemetry"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.telemetry_ingest.function_name
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_api_gateway_rest_api.telemetry_api.execution_arn}/*/POST/telemetry"
}

resource "aws_lambda_permission" "api_gateway_weather" {
  statement_id  = "AllowAPIGatewayWeather"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.weather_ingest.function_name
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_api_gateway_rest_api.telemetry_api.execution_arn}/*/POST/weather"
}
