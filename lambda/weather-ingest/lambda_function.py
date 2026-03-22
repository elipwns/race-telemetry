import json
import boto3
from decimal import Decimal
from datetime import datetime, timezone

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('weather-readings')

def lambda_handler(event, context):
    try:
        body = json.loads(event['body'])
        now = datetime.now(timezone.utc).isoformat()

        table.put_item(Item={
            'session_id':   body['session_id'],
            'timestamp':    now,
            'temp_c':       Decimal(str(body['temp_c'])),
            'humidity_pct': Decimal(str(body['humidity_pct'])),
            'pressure_hpa': Decimal(str(body.get('pressure_hpa', 0))),
        })

        return {'statusCode': 200, 'body': json.dumps({'status': 'ok'})}

    except Exception as e:
        print(f"ERROR: {e}")
        return {'statusCode': 500, 'body': json.dumps({'error': str(e)})}
