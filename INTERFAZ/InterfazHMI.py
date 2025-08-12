from pymodbus.client import ModbusTcpClient
from tkinter import Tk, Label, StringVar, Button, Frame, Canvas

# Configuración de la IP del ESP32 y el puerto Modbus
#ESP32_IP = '192.168.1.8'
ESP32_IP = '10.203.0.131'
MODBUS_PORT = 502
POTENTIOMETER_REGISTER = 0
TEMPERATURE_REGISTER = 1
FLUJO_REGISTER = 2
HUMEDAD_REGISTER = 3
NIVEL_AGUA_REGISTER = 4
PH_REGISTER = 5
STEP_REGISTER = 7  # Registro para enviar el paso actual al ESP32

client = ModbusTcpClient(ESP32_IP, port=MODBUS_PORT)

# Variables para la temperatura calibrada y ajustes
calibrated_temperature = 0.0
last_sensor_temperature = 0.0
temperature_adjustment = 0.0

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

# Función para iniciar el proceso de los indicadores
def start_process():
    activate_step(0)  # Iniciar el proceso desde el primer paso

# Función para activar cada paso en los indicadores y enviar el paso al ESP32
def activate_step(step):
    if step >= len(step_indicators):  # Detener el proceso si se completan todos los pasos
        return

    # Enviar el paso actual al ESP32
    if client.connect():
        client.write_register(STEP_REGISTER, step + 1)  # Enviar el paso actual (1-indexado)
        print(f"Paso {step + 1} enviado al ESP32")  # Confirmación en consola de Python
        client.close()
    
    # Encender el indicador del paso actual en verde
    step_indicators[step].itemconfig("indicator", fill="green")

    # Programar el siguiente paso después de 3 segundos
    root.after(3000, lambda: deactivate_step(step))

def deactivate_step(step):
    # Apagar el indicador del paso actual
    step_indicators[step].itemconfig("indicator", fill="gray")

    # Activar el siguiente paso
    activate_step(step + 1)

# Actualización de los valores leídos del ESP32
def update_values():
    global last_sensor_temperature
    if client.connect():
        pot_result = client.read_input_registers(POTENTIOMETER_REGISTER, 1)
        temp_result = client.read_input_registers(TEMPERATURE_REGISTER, 1)
        flujo_result = client.read_input_registers(FLUJO_REGISTER, 1)
        humedad_result = client.read_input_registers(HUMEDAD_REGISTER, 1)
        nivel_agua_result = client.read_input_registers(NIVEL_AGUA_REGISTER, 1)
        ph_result = client.read_input_registers(PH_REGISTER, 1)

        pot_value.set(f"Valor ADC: {pot_result.registers[0]}" if not pot_result.isError() else "Error de lectura")

        if not temp_result.isError():
            last_sensor_temperature = temp_result.registers[0] / 100.0
            temp_value.set(f"Temperatura: {last_sensor_temperature + temperature_adjustment:.2f} °C (Ajustada)")
        else:
            temp_value.set("Error de lectura")

        flujo_value.set(f"Flujo: {flujo_result.registers[0] / 100.0:.2f} L/min" if not flujo_result.isError() else "Error de lectura")
        humedad_value.set(f"Humedad: {humedad_result.registers[0]}" if not humedad_result.isError() else "Error de lectura")
        nivel_agua_value.set("Agua detectada!" if not nivel_agua_result.isError() and nivel_agua_result.registers[0] == 1 else "Sin agua.")
        ph_value.set(f"pH: {ph_result.registers[0] / 100.0:.2f}" if not ph_result.isError() else "Error de lectura")

        client.close()
    else:
        for value in [pot_value, temp_value, flujo_value, humedad_value, nivel_agua_value, ph_value]:
            value.set("Error de conexión")
        for canvas in step_indicators:
            canvas.itemconfig("indicator", fill="gray")

    root.after(500, update_values)

def update_temperature_display():
    temp_value.set(f"Temperatura: {last_sensor_temperature + temperature_adjustment:.2f} °C (Ajustada)")

# Configuración de la interfaz Tkinter
root = Tk()
root.title("Lectura de Sensores")
root.geometry("1920x1080")

# Variables para almacenar los valores
pot_value, temp_value, flujo_value, humedad_value, nivel_agua_value, ph_value = StringVar(), StringVar(), StringVar(), StringVar(), StringVar(), StringVar()
for value in [pot_value, temp_value, flujo_value, humedad_value, nivel_agua_value, ph_value]:
    value.set("Conectando...")

# Frames principales para los valores y los indicadores
sensor_frame = Frame(root)
sensor_frame.grid(row=0, column=0, padx=20, pady=20, sticky="n")

indicator_frame = Frame(root)
indicator_frame.grid(row=0, column=1, padx=20, pady=20, sticky="n")

# Título y etiquetas para los valores de los sensores
Label(sensor_frame, text="Monitor de Sensores", font=("Arial", 24, "bold")).grid(row=0, column=0, columnspan=2, pady=(0, 10))
Label(sensor_frame, text="Potenciómetro:", font=("Arial", 18)).grid(row=1, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=pot_value, font=("Arial", 18)).grid(row=1, column=1, padx=10, pady=10, sticky="w")
Label(sensor_frame, text="Termopar:", font=("Arial", 18)).grid(row=2, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=temp_value, font=("Arial", 18)).grid(row=2, column=1, padx=10, pady=10, sticky="w")
Button(sensor_frame, text="+1°C", font=("Arial", 14), command=increase_temperature).grid(row=2, column=2, padx=10)
Button(sensor_frame, text="-1°C", font=("Arial", 14), command=decrease_temperature).grid(row=2, column=3, padx=10)
Button(sensor_frame, text="Restablecer", font=("Arial", 14), command=reset_temperature).grid(row=2, column=4, padx=10)

Label(sensor_frame, text="Flujo:", font=("Arial", 18)).grid(row=3, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=flujo_value, font=("Arial", 18)).grid(row=3, column=1, padx=10, pady=10, sticky="w")
Label(sensor_frame, text="Humedad:", font=("Arial", 18)).grid(row=4, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=humedad_value, font=("Arial", 18)).grid(row=4, column=1, padx=10, pady=10, sticky="w")
Label(sensor_frame, text="Nivel de Agua:", font=("Arial", 18)).grid(row=5, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=nivel_agua_value, font=("Arial", 18)).grid(row=5, column=1, padx=10, pady=10, sticky="w")
Label(sensor_frame, text="pH:", font=("Arial", 18)).grid(row=6, column=0, padx=10, pady=10, sticky="e")
Label(sensor_frame, textvariable=ph_value, font=("Arial", 18)).grid(row=6, column=1, padx=10, pady=10, sticky="w")

# Botón de inicio de proceso
Button(root, text="Iniciar proceso", font=("Arial", 18), command=start_process).grid(row=1, column=0, pady=20)

# Título de indicadores
Label(indicator_frame, text="Indicadores", font=("Arial", 24, "bold")).grid(row=0, column=0, columnspan=3, pady=(0, 10))

# Indicadores grandes
indicators = ["Lavado", "Desagüe", "Remojo", "Centrifugado", "Descargue"]
for i, indicator in enumerate(indicators):
    Label(indicator_frame, text=indicator, font=("Arial", 20, "bold"), width=15).grid(row=i + 1, column=0, padx=10, pady=10)
    canvas = Canvas(indicator_frame, width=60, height=60)
    canvas.grid(row=i + 1, column=1, padx=5, pady=10)
    canvas.create_oval(10, 10, 50, 50, fill="gray", outline="black")

# Indicadores pequeños
small_indicator_frame = Frame(indicator_frame)
small_indicator_frame.grid(row=1, column=2, rowspan=5, padx=20, pady=20, sticky="n")

step_indicators = [Canvas(small_indicator_frame, width=30, height=30) for _ in range(11)]
for i, canvas in enumerate(step_indicators):
    Label(small_indicator_frame, text=f"Paso {i+1}", font=("Arial", 16), width=10).grid(row=i, column=0, padx=10, pady=5)
    canvas.grid(row=i, column=1, padx=5, pady=5)
    canvas.create_oval(5, 5, 25, 25, fill="gray", outline="black", tags="indicator")

# Inicia la actualización
update_values()

# Ejecuta la interfaz
root.mainloop()
