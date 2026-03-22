import json
import boto3
from decimal import Decimal
from datetime import datetime, timezone

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('telemetry-runs')

sns = boto3.client('sns')
SNS_TOPIC_ARN = 'arn:aws:sns:us-west-2:ACCOUNT_ID:telemetry-events'

def lambda_handler(event, context):
    try:
        body = json.loads(event['body'])
        now = datetime.now(timezone.utc).isoformat()

        table.put_item(Item={
            'session_id': body['session_id'],
            'timestamp':  now,
            'lat':        Decimal(str(body['lat'])),
            'lon':        Decimal(str(body['lon'])),
            'speed_kph':  Decimal(str(body['speed_kph'])),
            'heading':    body.get('heading', 0),
            'satellites': body.get('satellites', 0),
            'sequence':   body.get('sequence', 0),
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
