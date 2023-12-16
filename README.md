# Chuẩn bị
- điện thoại phát wifi
- xác định ip server (ip của máy tính)
- chạy mqtt server
# Nạp code
- cắm esp32-cam vào esp32-cam-mb
- nạp code từ Arduino
# Lắp PIR sensor
- nối chân 5v và GND của esp32-cam với 2 chân tương ứng của esp32-cam-mb
- nối chân dương của PIR với chân VCC của esp32-cam
- nối chân âm của PIR với chân âm của esp32-cam
- nối chân data của PIR với chân IO13 của esp32-cam
# cài đặt server
- `virtualenv env`
- `env/Scripts/activate`
- `pip install -r requirements.txt`
# chạy server
- đổi ip server tương ứng với ip wifi
- `python manager runserver <ip>:<port>`
