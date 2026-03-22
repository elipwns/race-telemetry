import json
import boto3

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('rt-websocket-connections')

def lambda_handler(event, context):
    try:
        connection_id = event['requestContext']['connectionId']
        table.delete_item(Key={'connectionId': connection_id})
        return {'statusCode': 200, 'body': 'Disconnected'}
    except Exception as e:
        print(f"ERROR: {e}")
        return {'statusCode': 200, 'body': json.dumps({'error': str(e)})}
