import json
import boto3
from datetime import datetime, timezone

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('rt-websocket-connections')

def lambda_handler(event, context):
    try:
        connection_id = event['requestContext']['connectionId']
        table.put_item(Item={
            'connectionId': connection_id,
            'timestamp': datetime.now(timezone.utc).isoformat()
        })
        return {'statusCode': 200, 'body': 'Connected'}
    except Exception as e:
        print(f"ERROR: {e}")
        return {'statusCode': 500, 'body': json.dumps({'error': str(e)})}
