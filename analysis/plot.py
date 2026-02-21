import serial
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import re
import csv
from datetime import datetime

ARDUINO_PORT = 'COM5'
BAUD_RATE = 9600

timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
csv_filename = f"dane_zelazko_{timestamp}.csv"

with open(csv_filename, mode='w', newline='') as file:
    writer = csv.writer(file, delimiter=',') 
    writer.writerow(["Czas_s", "Cel_C", "Obecna_C", "Moc_%"])

print(f"Utworzono plik zapisu: {csv_filename}")

x_time = []
y_setpoint = []
y_current = []
y_power = []

start_time = time.time()

print(f"Łączenie z {ARDUINO_PORT}...")

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) 
    ser.reset_input_buffer()
    print("Połączono!")
except Exception as e:
    print(f"Nie można otworzyć portu. \nSzczegóły: {e}")
    exit()

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
fig.suptitle(f'Analiza PID Żelazka - {timestamp}', fontsize=16)
plt.subplots_adjust(bottom=0.1, top=0.9, hspace=0.1)

def update(frame):
    data_received = False
    
    while ser.in_waiting:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line: continue

            numbers = re.findall(r"[-+]?\d*\.\d+|[-+]?\d+", line)

            if len(numbers) >= 3:
                val_set = float(numbers[0])
                val_curr = float(numbers[1])
                val_pwr = float(numbers[2])
                
                current_time = time.time() - start_time
                
                x_time.append(current_time)
                y_setpoint.append(val_set)
                y_current.append(val_curr)
                y_power.append(val_pwr)
            
                with open(csv_filename, mode='a', newline='') as file:
                    writer = csv.writer(file, delimiter=',')
                    
                    writer.writerow([
                        f"{current_time:.2f}", 
                        f"{val_set}", 
                        f"{val_curr}", 
                        f"{val_pwr}"
                    ])
                
                print(f"T={current_time:.1f}s | Cel: {val_set} | Temp: {val_curr} | Moc: {val_pwr}%")
                data_received = True

        except Exception as e:
            print(f"Błąd danych: {e}")

    if data_received:
        ax1.clear()
        ax1.plot(x_time, y_setpoint, label='Zadana [°C]', color='blue', linestyle='--', alpha=0.6)
        ax1.plot(x_time, y_current, label='Obecna [°C]', color='red', linewidth=2)
        ax1.set_ylabel('Temperatura [°C]')
        ax1.legend(loc='upper left')
        ax1.grid(True, linestyle=':', alpha=0.6)

        ax2.clear()
        ax2.fill_between(x_time, y_power, color='green', alpha=0.3)
        ax2.plot(x_time, y_power, label='Moc PID [%]', color='green', linewidth=1)
        ax2.set_ylabel('Moc [%]')
        ax2.set_xlabel('Czas [s]')
        ax2.set_ylim(0, 105) 
        ax2.legend(loc='upper left')
        ax2.grid(True, linestyle=':', alpha=0.6)

ani = animation.FuncAnimation(fig, update, interval=1000, cache_frame_data=False)

plt.show()
ser.close()