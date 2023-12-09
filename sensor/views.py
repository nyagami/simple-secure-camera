from datetime import datetime
import os
from django.conf import settings
from django.views.decorators.csrf import csrf_exempt
from django.shortcuts import render
from django.http import HttpResponse, HttpResponseBadRequest, HttpRequest
from .models import CameraHistory
import channels.layers
from asgiref.sync import async_to_sync
import json
# Create your views here.

def home(request):
    histories = [{
        'id': history.id,
        'captured_at': history.captured_at,
        'image_url': settings.MEDIA_URL + str(history.image)
    } for history in CameraHistory.objects.all().reverse()[:10]]

    return render(request, 'home.html', {
        'histories': histories
    })

@csrf_exempt
def captured(request: HttpRequest):
    if request.method == 'POST':
        try:
            image = request.FILES['media']
            new_file_name = 'captured' + '_' + str(datetime.now().timestamp()) + '.' + 'jpg'
            image_path = os.path.join(
                settings.MEDIA_ROOT, "capture", new_file_name
            )
            with open(image_path, 'wb') as f:
                f.write(image.read())
                f.close()
            history = CameraHistory.objects.create(image='capture/' + new_file_name)
            room_name = 'camera'
            channel_layer = channels.layers.get_channel_layer()
            async_to_sync(channel_layer.group_send)(room_name, {
                'type': 'captured',
                'message': json.dumps({
                    "id": history.id,
                    "captured_at": str(history.captured_at),
                    "image_url": settings.MEDIA_URL + str(history.image)
                })
            })
        except Exception as e:
            print('error', e)
            return HttpResponseBadRequest()
        return HttpResponse(json.dumps({'success': 'ok'}), content_type='application/json')
    return HttpResponseBadRequest()
