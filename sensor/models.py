import os
from datetime import datetime
from django.db import models
from django.conf import settings

def image_path(instance, filename):
    name, ext = str(filename).split('.')
    new_file_name = name + '_' + str(datetime.now().timestamp()) + '.' + ext
    return os.path.join(
        settings.MEDIA_ROOT, "capture", new_file_name
    )

# Create your models here.
class CameraHistory(models.Model):
    captured_at = models.DateTimeField(auto_now_add=True)
    image = models.ImageField(upload_to=image_path)

    def __str__(self) -> str:
        return self.image.name
