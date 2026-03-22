output "api_endpoint" {
  description = "REST API base URL — set in firmware CONFIG.md"
  value       = "https://${aws_api_gateway_rest_api.telemetry_api.id}.execute-api.${var.aws_region}.amazonaws.com/${aws_api_gateway_stage.prd.stage_name}"
}

output "websocket_endpoint" {
  description = "WebSocket URL — update in lambda/websocket-broadcast/lambda_function.py"
  value       = aws_apigatewayv2_stage.websocket_prod.invoke_url
}

output "websocket_api_id" {
  description = "WebSocket API ID — used to construct management endpoint"
  value       = aws_apigatewayv2_api.websocket.id
}

output "dashboard_url" {
  description = "Dashboard static site URL"
  value       = "http://${aws_s3_bucket_website_configuration.dashboard.website_endpoint}"
}

output "sns_topic_arn" {
  description = "SNS topic ARN — update in lambda/telemetry-ingest/lambda_function.py"
  value       = aws_sns_topic.telemetry_events.arn
}

output "data_analyst_role_arn" {
  description = "IAM role ARN for data scientist — use with AWS STS assume-role or boto3"
  value       = aws_iam_role.data_analyst.arn
}
