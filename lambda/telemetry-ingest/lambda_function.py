import json
import boto3
from decimal import Decimal
from datetime import datetime, timezone, timedelta

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('telemetry-runs')

sns = boto3.client('sns')
SNS_TOPIC_ARN = 'arn:aws:sns:us-west-2:772255980793:telemetry-events'

def lambda_handler(event, context):
    try:
        body = json.loads(event['body'])
        now = datetime.now(timezone.utc)
        expire_at = int((now + timedelta(days=90)).timestamp())
        now = now.isoformat()

        if body.get('satellites', 0) == 0:
            return {'statusCode': 200, 'body': json.dumps({'status': 'skipped', 'reason': 'no_fix'})}

        table.put_item(Item={
            'session_id': body['session_id'],
            'timestamp':  now,
            'lat':        Decimal(str(body['lat'])),
            'lon':        Decimal(str(body['lon'])),
            'speed_kph':  Decimal(str(body['speed_kph'])),
            'heading':    body.get('heading', 0),
            'satellites': body.get('satellites', 0),
            'sequence':   body.get('sequence', 0),
            'expire_at':  expire_at,
        })

        sns.publish(
            TopicArn=SNS_TOPIC_ARN,
            Message=json.dumps({
                'session_id': body['session_id'],
                'lat':        body['lat'],
                'lon':        body['lon'],
                'speed_kph':  body['speed_kph'],
                'heading':    body.get('heading', 0),
                'satellites': body.get('satellites', 0),
                'timestamp':  now,
            })
        )

        return {'statusCode': 200, 'body': json.dumps({'status': 'ok'})}

    except Exception as e:
        print(f"ERROR: {e}")
        return {'statusCode': 500, 'body': json.dumps({'error': str(e)})}
