from django.contrib import admin
from .models import CameraHistory

# Register your models here.
class CameraHistoryAdmin(admin.ModelAdmin):
    list_display = ['id', 'image', 'captured_at']

admin.site.register(CameraHistory, CameraHistoryAdmin)
