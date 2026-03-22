# SNS Topic for telemetry events → WebSocket broadcast

resource "aws_sns_topic" "telemetry_events" {
  name = "telemetry-events"
}

resource "aws_sns_topic_subscription" "websocket_broadcast" {
  topic_arn = aws_sns_topic.telemetry_events.arn
  protocol  = "lambda"
  endpoint  = aws_lambda_function.websocket_broadcast.arn
}

resource "aws_lambda_permission" "sns_websocket_broadcast" {
  statement_id  = "AllowSNSInvoke"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.websocket_broadcast.function_name
  principal     = "sns.amazonaws.com"
  source_arn    = aws_sns_topic.telemetry_events.arn
}
