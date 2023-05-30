# Import libraries
import paho.mqtt.client as mqtt
import tkinter as tk
import time
import RPi.GPIO as GPIO
import threading
import queue

# Set numbering mode for GPIO pins
GPIO.setmode(GPIO.BCM)

# Disable warning messages issued by RPi.GPIO library
GPIO.setwarnings(False)

# GPIO pin reference
in1 = 24
in2 = 23
en = 25

# Create Tkinter window and assign it to the variable "window"
window = tk.Tk()

# Hide window initially
window.withdraw()

# Calculate the center position of the screen
screen_width = window.winfo_screenwidth()
screen_height = window.winfo_screenheight()
window_width = 265
window_height = 75
x_position = (screen_width - window_width) // 2
y_position = (screen_height - window_height) // 2

# Set the window position
window.title("Continue watering")
window.geometry(f"{window_width}x{window_height}+{x_position}+{y_position}")

# Setup GPIO pins as output pins
GPIO.setup(in1, GPIO.OUT)
GPIO.setup(in2, GPIO.OUT)
GPIO.setup(en, GPIO.OUT)

# Set the output state of a GPIO pin to LOW
GPIO.output(in1, GPIO.LOW)
GPIO.output(in2, GPIO.LOW)

# Create object named "pwm" associated with GPIO pin "en" and has one-thousand hertz
pwn = GPIO.PWM(en, 1000)

# Start PWM output with duty cycle of one-hundred percent
pwn.start(100)

# Create motorOn function, turn motor on, and publish message
# "started" to topic "MotorStatus"
def motorOn():
    GPIO.output(in1, GPIO.HIGH)
    GPIO.output(in2, GPIO.LOW)
    client.publish("MotorStatus", "started")

def motorOff():
    GPIO.output(in1, GPIO.LOW)
    GPIO.output(in2, GPIO.LOW)
    client.publish("MotorStatus", "stopped")

# Create yes_action function, publish message "yes" to topic
# "Confirmation", and hide GUI window.
def yes_action():
    client.publish("Confirmation", "yes")
    window.withdraw()

def no_action():
    client.publish("Confirmation", "no")
    window.withdraw()

# Create showGUI function and show GUI window when called
def showGUI():
    window.deiconify()

def messageFunction(client, userdata, message):
    topic = str(message.topic)
    message = str(message.payload.decode("utf-8"))
    # print(topic + message)
    if message == "on":
        motorOn()
    if message == "off":
        motorOff()
    if message == "show":
        showGUI()

def mqtt_thread_function():
    global client
    client = mqtt.Client("project")
    client.connect("broker.emqx.io", 1883)
    client.subscribe("Gui")
    client.subscribe("Motor")
    client.subscribe("Confirmation")
    client.on_message = messageFunction
    client.loop_start()

# Create a thread for the MQTT client
mqtt_thread = threading.Thread(target=mqtt_thread_function)
mqtt_thread.start()

# Create a thread-safe queue for communication between threads
message_queue = queue.Queue()

def update_gui():
    while not message_queue.empty():
        message = message_queue.get()
        label.config(text=message)

    window.after(1000, update_gui)

# GUI setup
label = tk.Label(window, text="Would you like to re-water anyway?")
yes_button = tk.Button(window, text="Yes", command=yes_action)
no_button = tk.Button(window, text="No", command=no_action)

label.grid(row=0, column=0, columnspan=2)
yes_button.grid(row=1, column=0, padx=16, pady=16)
no_button.grid(row=1, column=1, padx=16, pady=16)

# Schedule the initial GUI update
window.after(0, update_gui)
window.mainloop()

# Main program loop
while (1):
    time.sleep(1)
