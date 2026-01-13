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
csv_filename = f"step_response_zelazko_{timestamp}.csv"

with open(csv_filename, mode='w', newline='') as file:
    writer = csv.writer(file, delimiter=',') 
    writer.writerow(["Time_s", "Input_u_PWM", "Output_y_Temp"])

print(f"--- ROZPOCZETO REJESTRACJE SKOKU ---")
print(f"Dane beda zapisywane do: {csv_filename}")
print(f"Format danych: Czas [s], Sterowanie [0-1000], Temperatura [C]")

x_time = []
y_input_u = []
y_output_y = [] 

start_time = time.time()

print(f"Laczenie z {ARDUINO_PORT}...")

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
    ser.reset_input_buffer()
    print("Polaczono! Oczekiwanie na dane...")
except Exception as e:
    print(f"BLAD: Nie mozna otworzyc portu. Zamknij Arduino IDE! \nSzczegoly: {e}")
    exit()

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
fig.suptitle(f'Identyfikacja Obiektu (Skok) - {timestamp}', fontsize=16)
plt.subplots_adjust(bottom=0.1, top=0.9, hspace=0.2)

def update(frame):
    data_received = False
    
    while ser.in_waiting:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line: continue

            numbers = re.findall(r"[-+]?\d*\.\d+|[-+]?\d+", line)

            if len(numbers) >= 2:
                val_u = float(numbers[0]) # Wejscie (Sterowanie)
                val_y = float(numbers[1]) # Wyjscie (Temperatura)
                
                current_time = time.time() - start_time
                
                x_time.append(current_time)
                y_input_u.append(val_u)
                y_output_y.append(val_y)
            
                with open(csv_filename, mode='a', newline='') as file:
                    writer = csv.writer(file, delimiter=',')
                    writer.writerow([
                        f"{current_time:.3f}", 
                        f"{val_u}", 
                        f"{val_y}"
                    ])
                
                print(f"T={current_time:.1f}s | Wejscie u(t): {val_u} | Wyjscie y(t): {val_y}°C")
                data_received = True

        except Exception as e:
            print(f"Blad parsowania: {e}")

    if data_received:
        ax1.clear()
        ax1.plot(x_time, y_output_y, label='Wyjscie y(t) - Temperatura', color='red', linewidth=2)
        ax1.set_ylabel('Temperatura [°C]')
        ax1.set_title("Odpowiedz ukladu")
        ax1.legend(loc='upper left')
        ax1.grid(True, linestyle=':', alpha=0.6)

        ax2.clear()
        ax2.plot(x_time, y_input_u, label='Wesjcie u(t) - PWM (0-1000)', color='blue', linewidth=1.5)
        ax2.fill_between(x_time, y_input_u, color='blue', alpha=0.1)
        ax2.set_ylabel('Sygnal sterujacy')
        ax2.set_xlabel('Czas [s]')
        ax2.set_title("Wymuszenie (Skok jednostkowy)")

        if len(y_input_u) > 0:
            max_u = max(y_input_u)
            if max_u == 0: max_u = 100
            ax2.set_ylim(-10, max_u * 1.2)
        
        ax2.legend(loc='upper left')
        ax2.grid(True, linestyle=':', alpha=0.6)

ani = animation.FuncAnimation(fig, update, interval=500, cache_frame_data=False)

plt.show()
ser.close()
print("Zakonczono. Plik CSV jest gotowy do Matlaba.")