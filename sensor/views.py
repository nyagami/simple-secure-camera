from datetime import datetime
import os
import time
from django.conf import settings
from django.views.decorators.csrf import csrf_exempt
from django.shortcuts import render
from django.http import HttpResponse, HttpResponseBadRequest, HttpRequest
from .models import CameraHistory
import channels.layers
from asgiref.sync import async_to_sync
import json
from paho.mqtt import client as mqtt_client
from PIL import Image
from io import BytesIO

# Create your views here.

def on_connect(mqtt_client, userdata, flags, rc):
    if rc == 0:
       print('MQTT Connected successfully')
       for topic in settings.MQTT_TOPICS:
        mqtt_client.subscribe(topic)
    else:
        print('Bad connection. Code:', rc)

def on_message(mqtt_client, userdata, msg):
    if msg.topic == 'status':
        room_name = 'camera'
        channel_layer = channels.layers.get_channel_layer()
        async_to_sync(channel_layer.group_send)(room_name, {
            'type': 'captured',
            'message': json.dumps({
                'type': 'status',
                'status': msg.payload.decode()
            })
        })
    elif msg.topic == 'captured':
        img = Image.open(BytesIO(msg.payload))
        new_file_name = 'captured' + '_' + str(datetime.now().timestamp()) + '.' + 'jpg'
        image_path = os.path.join(
            settings.MEDIA_ROOT, "capture", new_file_name
        )
        img.save(image_path)
        history = CameraHistory.objects.create(image='capture/' + new_file_name)
        room_name = 'camera'
        channel_layer = channels.layers.get_channel_layer()
        async_to_sync(channel_layer.group_send)(room_name, {
            'type': 'captured',
            'message': json.dumps({
                'type': 'captured',
                "id": history.id,
                "captured_at": str(history.captured_at),
                "image_url": settings.MEDIA_URL + str(history.image)
            })
        })
    else:
        pass
    print(f'Received message on topic: {msg.topic}')

client = mqtt_client.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(
   host=settings.MQTT_SERVER,
   port=settings.MQTT_PORT,
)
client.loop_start()


def home(request):
    histories = [{
        'id': history.id,
        'captured_at': history.captured_at,
        'image_url': settings.MEDIA_URL + str(history.image)
    } for history in CameraHistory.objects.all().order_by('-id')[:10]]

    return render(request, 'home.html', {
        'histories': histories
    })

@csrf_exempt
def action(request):
    checked = request.POST.get('checked')
    if checked:
        client.publish('action', '1')
    else:
        client.publish('action', '0')
    return HttpResponse(json.dumps({'success': 'ok'}), content_type='application/json')
