import json
import boto3

dynamodb = boto3.resource('dynamodb')
connections_table = dynamodb.Table('rt-websocket-connections')

# Update this after terraform apply (from outputs.tf)
WEBSOCKET_ENDPOINT = 'https://REPLACE_ME.execute-api.us-west-2.amazonaws.com/prod'

apigateway = boto3.client('apigatewaymanagementapi', endpoint_url=WEBSOCKET_ENDPOINT)

def lambda_handler(event, context):
    message = json.loads(event['Records'][0]['Sns']['Message'])

    response = connections_table.scan()
    connections = response.get('Items', [])

    for connection in connections:
        try:
            apigateway.post_to_connection(
                ConnectionId=connection['connectionId'],
                Data=json.dumps(message).encode('utf-8')
            )
        except:
            connections_table.delete_item(Key={'connectionId': connection['connectionId']})

    return {'statusCode': 200}
