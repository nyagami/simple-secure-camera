from django.urls import path
from . import views, consumers

urlpatterns = [
    path('', views.home, name='home'),
    path("api/captured/", views.captured, name='captured')
]


websocket_urlpatterns = [
    path("ws/camera/", consumers.ChatConsumer.as_asgi()),
]
