from pymodbus.client import ModbusTcpClient
from tkinter import Tk, Label

# Configuración de la IP del ESP32 y el puerto Modbus
ESP32_IP = '192.168.1.8'
MODBUS_PORT = 502
MESSAGE_REGISTER = 7  # Registro donde se enviará el mensaje

client = ModbusTcpClient(ESP32_IP, port=MODBUS_PORT)

# Configuración de la interfaz Tkinter
root = Tk()
root.title("Prueba de comunicación")
root.geometry("300x100")

# Etiqueta para mostrar el estado de la conexión
status_label = Label(root, text="", font=("Arial", 14))
status_label.pack(pady=20)

# Función para enviar mensaje al ESP32 automáticamente
def send_message():
    if client.connect():
        client.write_register(MESSAGE_REGISTER, 1)  # Enviar señal al ESP32
        status_label.config(text="Conexión exitosa, mensaje enviado.")
        client.close()
    else:
        status_label.config(text="Error de conexión con ESP32.")

# Enviar el mensaje automáticamente al iniciar
send_message()

# Ejecuta la interfaz
root.mainloop()
