from pymodbus.client import ModbusTcpClient
from tkinter import Tk, Label, StringVar, Button, Frame, Canvas
import serial

# Configuración de las IPs y el puerto Modbus
ESP32_EMISOR_IP = '192.168.1.7'  # IP del ESP32 emisor (lectura de datos)
MODBUS_PORT = 502
POTENTIOMETER_REGISTER = 0
TEMPERATURE_REGISTER = 1
FLUJO_REGISTER = 2
FLUJO2_REGISTER = 6  # Registro Modbus para el segundo sensor de flujo
HUMEDAD_REGISTER = 3
NIVEL_AGUA_REGISTER = 4
PH_REGISTER = 5

# Configuración del puerto Serial para el Arduino UNO
ARDUINO_SERIAL_PORT = "COM3"  # Cambia por el puerto COM correspondiente
ARDUINO_BAUDRATE = 9600

# Cliente Modbus para emisor
client_emisor = ModbusTcpClient(ESP32_EMISOR_IP, port=MODBUS_PORT)

# Inicializar conexión serial con Arduino UNO
try:
    arduino_serial = serial.Serial(ARDUINO_SERIAL_PORT, ARDUINO_BAUDRATE, timeout=1)
except serial.SerialException as e:
    print(f"Error al conectar con el Arduino UNO: {e}")
    arduino_serial = None

# Variables para la temperatura calibrada y ajustes
calibrated_temperature = 0.0
last_sensor_temperature = 0.0
temperature_adjustment = 0.0

# Variables para el proceso de pasos
current_step = 0
is_process_running = False

# Variables para el estado del PIN 13
pin13_state = False  # Inicialmente apagado

# Funciones para ajuste de temperatura
def increase_temperature():
    global temperature_adjustment
    temperature_adjustment += 1.0
    update_temperature_display()

def decrease_temperature():
    global temperature_adjustment
    temperature_adjustment -= 1.0
    update_temperature_display()

def reset_temperature():
    global temperature_adjustment
    temperature_adjustment = 0.0
    update_temperature_display()

# Función para iniciar el proceso
def start_process():
    global current_step, is_process_running
    if not is_process_running:
        current_step = 0
        is_process_running = True
        execute_step()

# Lista de tiempos personalizados por paso en milisegundos
step_intervals = [5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000]

def execute_step():
    global current_step, is_process_running, can_execute_step_11
    if current_step < len(step_indicators):
        # Antes de ejecutar el paso 11, verificar si está permitido continuar
        if current_step == 10 and not can_execute_step_11:
            print("Esperando confirmación para el paso 11...")
            return  # Pausar el proceso hasta que el botón sea presionado

        # Enviar comando al Arduino para activar el pin correspondiente
        if arduino_serial and arduino_serial.is_open:
            arduino_serial.write(f"PASO_{current_step + 1}\n".encode())
            print(f"Enviado: PASO_{current_step + 1}")

        # Actualizar indicadores en la interfaz
        for i, canvas in enumerate(step_indicators):
            color = "green" if i == current_step else "gray"
            canvas.itemconfig("indicator", fill=color)

        # Programar el siguiente paso después del intervalo específico
        root.after(step_intervals[current_step], lambda: next_step())
    else:
        is_process_running = False
        print("Proceso completado.")

def next_step():
    global current_step
    current_step += 1
    execute_step()

def allow_step_11():
    global can_execute_step_11
    can_execute_step_11 = True  # Permitir ejecutar el paso 11
    print("Confirmación recibida: paso 11 habilitado")
    execute_step()  # Reanudar el proceso

# Actualización de los valores leídos del ESP32 emisor
def update_values():
    global last_sensor_temperature
    if client_emisor.connect():
        pot_result = client_emisor.read_input_registers(POTENTIOMETER_REGISTER, 1)
        temp_result = client_emisor.read_input_registers(TEMPERATURE_REGISTER, 1)
        flujo_result = client_emisor.read_input_registers(FLUJO_REGISTER, 1)
        flujo2_result = client_emisor.read_input_registers(FLUJO2_REGISTER, 1)  # Leer segundo sensor de flujo
        humedad_result = client_emisor.read_input_registers(HUMEDAD_REGISTER, 1)
        nivel_agua_result = client_emisor.read_input_registers(NIVEL_AGUA_REGISTER, 1)
        ph_result = client_emisor.read_input_registers(PH_REGISTER, 1)

        pot_value.set(f"ADC: {pot_result.registers[0]}" if not pot_result.isError() else "Error")
        temp_value.set(f"Temp: {temp_result.registers[0] / 100.0 + temperature_adjustment:.2f}°C" if not temp_result.isError() else "Error")
        flujo_value.set(f"Flujo: {flujo_result.registers[0] / 100.0:.2f} L/min" if not flujo_result.isError() else "Error")
        flujo2_value.set(f"Flujo 2: {flujo2_result.registers[0] / 100.0:.2f} L/min" if not flujo2_result.isError() else "Error")  # Actualizar flujo 2
        humedad_value.set(f"Humedad: {humedad_result.registers[0]}" if not humedad_result.isError() else "Error")
        nivel_agua_value.set("Agua: Sí" if not nivel_agua_result.isError() and nivel_agua_result.registers[0] == 1 else "Agua: No")
        ph_value.set(f"pH: {ph_result.registers[0] / 100.0:.2f}" if not ph_result.isError() else "Error")

        client_emisor.close()
    else:
        for value in [pot_value, temp_value, flujo_value, flujo2_value, humedad_value, nivel_agua_value, ph_value]:
            value.set("Error de conexión")

    root.after(500, update_values)

def update_temperature_display():
    temp_value.set(f"Temp: {last_sensor_temperature + temperature_adjustment:.2f}°C")

# Función para alternar el estado del PIN 13
def toggle_pin13():
    global pin13_state
    if arduino_serial and arduino_serial.is_open:
        if pin13_state:  # Si está encendido, lo apagamos
            arduino_serial.write("APAGAR_PIN_13\n".encode())
            print("Comando enviado: APAGAR_PIN_13")
        else:  # Si está apagado, lo encendemos
            arduino_serial.write("ENCENDER_PIN_13\n".encode())
            print("Comando enviado: ENCENDER_PIN_13")
        pin13_state = not pin13_state  # Cambiamos el estado
    else:
        print("Error: El puerto Serial no está abierto.")

# Configuración de la interfaz Tkinter
root = Tk()
root.title("Monitor de Sensores")
root.geometry("600x400")  # Tamaño más compacto

# Variables para almacenar los valores
pot_value, temp_value, flujo_value, flujo2_value, humedad_value, nivel_agua_value, ph_value = StringVar(), StringVar(), StringVar(), StringVar(), StringVar(), StringVar(), StringVar()
for value in [pot_value, temp_value, flujo_value, flujo2_value, humedad_value, nivel_agua_value, ph_value]:
    value.set("Conectando...")

# Frame principal
main_frame = Frame(root, padx=10, pady=10)
main_frame.pack(fill="both", expand=True)

# Frame de sensores
sensor_frame = Frame(main_frame, relief="ridge", borderwidth=2, padx=5, pady=5)
sensor_frame.grid(row=0, column=0, sticky="n")

# Título y etiquetas de sensores
Label(sensor_frame, text="Sensores", font=("Arial", 14, "bold")).grid(row=0, column=0, columnspan=2, pady=5)
#Label(sensor_frame, textvariable=pot_value, font=("Arial", 12)).grid(row=1, column=0, padx=5, pady=5, sticky="w")
Label(sensor_frame, textvariable=temp_value, font=("Arial", 12)).grid(row=2, column=0, padx=5, pady=5, sticky="w")
Label(sensor_frame, textvariable=flujo_value, font=("Arial", 12)).grid(row=3, column=0, padx=5, pady=5, sticky="w")
Label(sensor_frame, textvariable=flujo2_value, font=("Arial", 12)).grid(row=4, column=0, padx=5, pady=5, sticky="w")  # Etiqueta para flujo 2
Label(sensor_frame, textvariable=humedad_value, font=("Arial", 12)).grid(row=5, column=0, padx=5, pady=5, sticky="w")
Label(sensor_frame, textvariable=nivel_agua_value, font=("Arial", 12)).grid(row=6, column=0, padx=5, pady=5, sticky="w")
Label(sensor_frame, textvariable=ph_value, font=("Arial", 12)).grid(row=7, column=0, padx=5, pady=5, sticky="w")

# Frame de ajustes y botones
adjust_frame = Frame(main_frame, relief="ridge", borderwidth=2, padx=5, pady=5)
adjust_frame.grid(row=0, column=1, sticky="n")

Label(adjust_frame, text="Ajustes", font=("Arial", 14, "bold")).grid(row=0, column=0, columnspan=2, pady=5)
Button(adjust_frame, text="+1°C", font=("Arial", 10), command=increase_temperature).grid(row=1, column=0, padx=5, pady=5)
Button(adjust_frame, text="-1°C", font=("Arial", 10), command=decrease_temperature).grid(row=1, column=1, padx=5, pady=5)
Button(adjust_frame, text="Reset", font=("Arial", 10), command=reset_temperature).grid(row=2, column=0, columnspan=2, pady=5)
Button(adjust_frame, text="Iniciar Proceso", font=("Arial", 12), command=start_process).grid(row=3, column=0, columnspan=2, pady=10)

# Botón para encender y apagar el PIN 13
Button(adjust_frame, text="Encender/Apagar PIN 13", font=("Arial", 12), command=toggle_pin13).grid(row=5, column=0, columnspan=2, pady=5)

Button(adjust_frame, text="Termino el descargue", font=("Arial", 12), command=allow_step_11).grid(row=6, column=0, columnspan=2, pady=10)

# Indicadores compactos
indicator_frame = Frame(main_frame, relief="ridge", borderwidth=2, padx=5, pady=5)
indicator_frame.grid(row=1, column=0, columnspan=2, pady=10, sticky="ew")

Label(indicator_frame, text="Pasos", font=("Arial", 14, "bold")).grid(row=0, column=0, columnspan=3, pady=5)

step_indicators = [Canvas(indicator_frame, width=20, height=20) for _ in range(11)]
for i, canvas in enumerate(step_indicators):
    Label(indicator_frame, text=f"{i+1}", font=("Arial", 10)).grid(row=i // 6 + 1, column=(i % 6) * 2, padx=5)
    canvas.grid(row=i // 6 + 1, column=(i % 6) * 2 + 1, padx=5)
    canvas.create_oval(5, 5, 15, 15, fill="gray", outline="black", tags="indicator")

# Inicia la actualización
update_values()

# Ejecuta la interfaz
root.mainloop()
