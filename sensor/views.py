from django.conf import settings
from django.shortcuts import render
from .models import CameraHistory
# Create your views here.

def home(request):
    histories = [{
        'id': history.id,
        'captured_at': history.captured_at,
        'image_url': settings.MEDIA_URL + str(history.image)
    } for history in CameraHistory.objects.all()[:10]]

    return render(request, 'home.html', {
        'histories': histories
    })