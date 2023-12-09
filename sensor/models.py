from django.db import models
from django.conf import settings

# Create your models here.
class CameraHistory(models.Model):
    captured_at = models.DateTimeField(auto_now_add=True)
    image = models.ImageField(upload_to=settings.MEDIA_ROOT)
