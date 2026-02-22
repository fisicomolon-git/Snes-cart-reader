#genera el ejecutable con esta orden en cosola en la carpeta donde esté el py: 
#  pyinstaller --onefile --icon=icon.ico --noconsole "descargar rom 0.93.py"

import serial
import serial.tools.list_ports
import threading
import tkinter as tk
import time
from tkinter import messagebox
from tkinter import filedialog
import math 
from tkinter import ttk
import itertools
import os

waiting = 0 #variable global
nombre= None
sramsize=0
baudios = 250000

def animate_text(etiqueta):
    states = ["Dumping", "Dumping.", "Dumping..", "Dumping..."]
    for state in itertools.cycle(states):
        if not running:
            break
        etiqueta.config(text=state)
        time.sleep(0.5)



###DUMP RAM###
def DUMP_RAM(puerto, baudrate, archivo_salida, inicio, cierre):
    """Función para escuchar el puerto serie en un hilo separado."""
    global nombre
    global running
    global region
    if nombre:
        try:
            with serial.Serial(puerto, baudrate, timeout=1) as ser:
                time.sleep(2)
      
                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
                    time.sleep(0.05)  # da tiempo a que terminen de llegar bytes

                # Esperar un poco más, por si el micro manda más datos al arrancar
                time.sleep(2)
                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
    
                
                ser.write(b'c')  # Envía el carácter 'C'
                print("Carácter 'c' enviado")
                print(f"Escuchando en {puerto} a {baudrate} baudios...")
                archivo = None
                escribiendo = False
                leyendo = True

                while leyendo:
                    linea = ser.readline()
                    if linea.strip(b'\x00'):  # Ignorar bytes vacíos
                        texto = linea.decode('utf-8', errors='ignore').strip()
                        if texto == inicio and not escribiendo:
                            archivo = open(archivo_salida, 'w')
                            escribiendo = True
                            start_time = time.time()
                            boton_enviar["state"] = "disabled"
                            boton_iniciar["state"] = "disabled"
                            boton_iniciar_sram["state"] = "disabled"
                            boton_enviar_sram["state"] = "disabled"
                            print("inicio")

                            ###inicio animacion de carga
                            global running
                            running = True
                            threading.Thread(target=animate_text, args=(etiqueta,), daemon=True).start()
                            ##########################
                            continue

                        if texto == cierre and escribiendo:
                            archivo.close()
                            escribiendo = False
                            leyendo = False
                            print("fin")
                            time.sleep(2)
                            ser.close()
                            os.makedirs('roms', exist_ok=True) #crea la carpeta sino existe
                            # Llama a la función con los nombres de los archivos
                            convertir_texto_a_binario_memoria('.\\'+nombre+'_srm.txt', '.\\roms\\'+nombre+'('+region+')'+'.srm')
                            ############paro animación de carga
                            
                            running = False
                            ####################
                            nombre= None
                            finish_time = time.time()
                            total_time = finish_time - start_time
                            #total_time = 540
                            min =  math.trunc(total_time/60)
                            sec = round((total_time - (min*60)),)
                            messagebox.showinfo('DUMP DONE', "The SRAM has been succesfully dumped \n in "+str(min)+" minutes and "+str(sec)+" seconds")
                            etiqueta2.config(text=f"No info obtained from cart")                                              # label error
                            etiqueta2.config(fg="red")
                            boton_enviar["state"] = "active"
                            continue

                        if escribiendo:
                            archivo.write(texto + '\n')
                            archivo.flush()
        except serial.SerialException as e:
            nombre = None
            messagebox.showerror(title="ERROR", message="Error comunicating withe cart reader. Did you unplug the reader?", )









## CART INFO##
def cart_info():
    """Envía un carácter específico por el puerto serie y recibe 21 bytes."""
    global puerto_ch340
    global nombre
    global sramsize
    global region
    puerto_ch340 = detectar_puerto()
    if puerto_ch340:
        try:
            with serial.Serial(puerto_ch340, baudios, timeout=1) as ser:
                time.sleep(2)
                '''
                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
                    time.sleep(0.05)  # da tiempo a que terminen de llegar bytes

                # Esperar un poco más, por si el micro manda más datos al arrancar
                time.sleep(2)
                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
                '''

                ser.reset_input_buffer()
                ser.write(b'b')  # Envía el carácter 'b'
                bytes_recibidos = ser.readline() # si el boton está pulsado hacia arriba, como tiene que estar, al leer el puerto serie, primero manda el nombre del cartucho y luego ya la info.
                print (bytes_recibidos)
                #bytes_recibidos = ser.readline() #vuelvo a leer la siguiente linea machacando la anterior que era el codigo para lanzar el juego
                ser.close()
                print (bytes_recibidos)

                palabra= bytes_recibidos.decode("utf-8")
                palabra_list = palabra.rsplit("*") #partimos por * que he usado como separador entre bytes en el codigo de arduino.
                nombre=""

                #print (palabra_list)
                romtype= (palabra_list[0]) #lo primero que recibo es el hirom or lorom
                romsizekB= (palabra_list[1]) #luego recibo el tamaño de la rom en KB kilobiytes
                romsizeMb= (palabra_list[2]) #luego recibo el tamaño de la rom en Mb megabits
                sramsize= (palabra_list[3]) #guardo el numero para luego usarlo en otro def
                sramsizekB= (palabra_list[3])+' kbytes - ' #luego recibo el tamaño de la sram en kilobytes
                sramsizekb= (palabra_list[4])+' kbits' #luego recibo el tamaño de la sram en kilobits
                if sramsizekB == '1 kbytes - ':
                    sramsizekB= "NO SRAM"
                    sramsizekb= ""


                region = (palabra_list[5]) #luego recibo la region
                palabra_list = palabra_list[6:27] #elimino estos 6 primeros elementos de la lista y me quedo con los 21 restantes
                if int(palabra_list[0])>123 : #el texto del nombre  es japones
                    
                    # Filtrar los valores vacíos antes de procesarlos
                    palabra_list = [x for x in palabra_list if x.strip()]

                    # Convertimos los valores numéricos a bytes (de los caracteres en japonés)
                    byte_data = bytes(int(x) for x in palabra_list)

                    # Intentamos decodificar correctamente en Shift_JIS
                    try:
                        nombre = byte_data.decode("shift_jis").strip()
                    except UnicodeDecodeError:
                        nombre = byte_data.decode("utf-8", errors="replace").strip()

                    #print("Texto decodificado:", nombre)  # DEBUG: Verificar en consola
                
                else: #el texto del nombre es normal, no japones
                    n=0
                    for i in palabra_list:
                        #print (palabra_list[n])ca
                        nombre= nombre + chr(int(palabra_list[int(n)]))
                        n=n+1
                    nombre = nombre.strip() #limpio espacios en blanco


                if palabra_list[0]=='0': #este es la primera letra del nombre del juego si es 0 (48 ascii) es que no hay cartucho.
                    nombre= None
                    messagebox.showerror(title="ERROR", message="No cart detected, Did you blow the cart?", )
                    etiqueta2.config(text=f"No info obtained from cart")                                              # label error
                    etiqueta2.config(fg="red")

                #print (bytes_recibidos)
                #print (bytes_recibidos[15])

                if palabra_list[0]!='0': #ningun nombre comienza por 0 (48 ascii) así que si es distinto es que algún nombre ha leido
                    lines = ['Name: '+nombre, 'Rom type: '+romtype, 'Rom size: '+romsizekB+' kbytes - '+ romsizeMb+' Mbits', 'Sram size: '+sramsizekB+ ' '+sramsizekb, 'Region: '+region, 'Time needed to dump: '+str(int(romsizeMb)*0.625)+' min. aprox.' ]
                    print (lines)
                    messagebox.showinfo('Cart Info', "\n".join(lines))
                    boton_iniciar["state"] = "active"
                    #print (sramsize)
                    if sramsize!="1":
                        #print (sramsize)
                        boton_iniciar_sram["state"] = "active"
                        boton_enviar_sram["state"] = "active"
                    else:
                        boton_iniciar_sram["state"] = "disabled"
                        boton_enviar_sram["state"] = "disabled" 
                        

                    etiqueta2.config(text=f"Cartridge information successfully obtained. ")  # label exito
                    etiqueta2.config(fg="green") 

                    

        except serial.SerialException as e:
            messagebox.showerror(title="ERROR", message="Error comunicating withe cart reader. Close other programs", )


###DUMP ROM###
def DUMP(puerto, baudrate, archivo_salida, inicio, cierre):
    """Función para escuchar el puerto serie en un hilo separado."""
    global nombre
    global running
    if nombre:
        try:
            with serial.Serial(puerto, baudrate, timeout=1) as ser:
                time.sleep(2)

                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
                    time.sleep(0.05)  # da tiempo a que terminen de llegar bytes

                # Esperar un poco más, por si el micro manda más datos al arrancar
                time.sleep(0.5)
                while ser.in_waiting > 0:
                    ser.read(ser.in_waiting)

                ser.write(b'a')  # Envía el carácter 'a'
                print("Carácter 'a' enviado")
                print(f"Escuchando en {puerto} a {baudrate} baudios...")
                archivo = None
                escribiendo = False
                leyendo = True

                while leyendo:
                    linea = ser.readline()
                    if linea.strip(b'\x00'):  # Ignorar bytes vacíos
                        texto = linea.decode('utf-8', errors='ignore').strip()
                        if texto == inicio and not escribiendo:
                            archivo = open(archivo_salida, 'wb')
                            escribiendo = True
                            start_time = time.time()
                            boton_enviar["state"] = "disabled"
                            boton_iniciar["state"] = "disabled"
                            boton_iniciar_sram["state"] = "disabled"
                            boton_enviar_sram["state"] = "disabled"
                            print("inicio")

                            ###inicio animacion de carga
                            global running
                            running = True
                            ##threading.Thread(target=animate_text, args=(etiqueta,), daemon=True).start()
                            ##########################
                            continue

                        if texto == cierre and escribiendo:
                            archivo.close()
                            escribiendo = False
                            leyendo = False
                            print("fin")
                            time.sleep(2)
                            ser.close()
                            # Llama a la función con los nombres de los archivos
                            os.makedirs('roms', exist_ok=True) #crea la carpeta sino existe
                            convertir_texto_a_binario_memoria('.\\'+nombre+'.txt', '.\\roms\\'+nombre+'('+region+')'+'.sfc')
                            ############paro animación de carga
                            
                            running = False
                            ####################
                            nombre= None
                            finish_time = time.time()
                            total_time = finish_time - start_time
                            #total_time = 540
                            min =  math.trunc(total_time/60)
                            sec = round((total_time - (min*60)),)
                            messagebox.showinfo('DUMP DONE', "The cartridge has been succesfully dumped \n in "+str(min)+" minutes and "+str(sec)+" seconds")
                            etiqueta2.config(text=f"No info obtained from cart")                                              # label error
                            etiqueta2.config(fg="red")
                            boton_enviar["state"] = "active"
                            continue

                        if escribiendo:
                            #archivo.write(texto + '\n')
                            archivo.write(linea)
                            archivo.flush()
        except serial.SerialException as e:
            nombre = None
            messagebox.showerror(title="ERROR", message="Error comunicating withe cart reader. Did you unplug the reader?", )

def detectar_puerto():
    """Detecta el puerto CH340 y actualiza la interfaz."""
    global puerto_ch340
    puertos_disponibles = serial.tools.list_ports.comports()
    for puerto in puertos_disponibles:
        
        if "usb-serial ch340" in puerto.description.lower() or "usb serial port" in puerto.description.lower():

            puerto_ch340 = puerto.device
            etiqueta.config(text=f"Cart reader detected in {puerto_ch340}") #label exito
            etiqueta.config(fg="green") 
            return puerto_ch340 
        
    messagebox.showerror(title="ERROR", message="No cart reader detected, insert cart reader in USB port", )
    etiqueta.config(text="No cart reader detected, insert cart reader in USB port")
    etiqueta.config(fg="red")   
    return None

def start_dump():
    """Inicia la lectura del puerto serie en un hilo separado.""" 
    global puerto_ch340
    global nombre
    puerto_ch340 = detectar_puerto()

    if puerto_ch340:
        if nombre:
            hilo_serial = threading.Thread(target=DUMP, args=(puerto_ch340, baudios, ".\\"+nombre+'.txt', "<", ">"), daemon=True)
            hilo_serial.start()
        else:
            messagebox.showerror(title="ERROR", message="Get cart info first", )
            etiqueta2.config(text=f"No info obtained from cart")                                              # label error
            etiqueta2.config(fg="red")



def start_dump_sram():
    """Inicia la lectura del puerto serie en un hilo separado.""" 
    global puerto_ch340
    global nombre
    puerto_ch340 = detectar_puerto()

    if puerto_ch340:
        if nombre:
            hilo_serial = threading.Thread(target=DUMP_RAM, args=(puerto_ch340, baudios, ".\\"+nombre+'_srm.txt', "<", ">"), daemon=True)
            hilo_serial.start()
        else:
            messagebox.showerror(title="ERROR", message="Get cart info first", )
            etiqueta2.config(text=f"No info obtained from cart")                                              # label error
            etiqueta2.config(fg="red")
    




def convertir_texto_a_binario_memoria(input_file, output_file):
    with open(input_file, 'r') as infile:
        # Carga todo el contenido del archivo en memoria y lo divide en líneas
        lines = infile.read().splitlines()
    
    # Convierte todas las líneas a bytes y las almacena en una lista
    bytes_list = []
    for line in lines:
        line = line.strip()  # Limpia espacios y saltos de línea
        if line:  # Ignora líneas vacías
            # Convierte el texto (en base 16 o base 10) a un entero y lo guarda como byte
            byte = int(line, 16)  # Cambiar a base 10 si el archivo usa decimales
            bytes_list.append(byte)
    
    # Escribe todos los bytes al archivo binario de una vez
    with open(output_file, 'wb') as outfile:
        outfile.write(bytes(bytes_list))




def send_sram():
    global sramsizekB
    while True:
        contador=0
        global puerto_ch340
        archivo = filedialog.askopenfilename(title="Seleccionar un archivo")
        if archivo:  # Si se seleccionó un archivo
            nombre_archivo.set(archivo)  # Guardar la ruta en la variable
        
        
        size_real = os.path.getsize(archivo)
        if size_real != (int(sramsize)*1024):
            #print(int(sramsize)*1024)
            messagebox.showerror(title="ERROR", message="Wrong file size. Check the srm file with an emulator", )
            break

        file_path = nombre_archivo.get() # Cambia esto por el archivo que quieras enviar
        serial_port = puerto_ch340  # Cambia esto según tu sistema (ej. "/dev/ttyUSB0" en Linux)
        baud_rate = baudios  # Puedes cambiar la velocidad del puerto serie
        try:
            # Abre el puerto serie
            with serial.Serial(serial_port, baudios, timeout=1) as ser:
                start_time = time.time()
                boton_enviar["state"] = "disabled"
                boton_iniciar["state"] = "disabled"
                boton_iniciar_sram["state"] = "disabled"
                boton_enviar_sram["state"] = "disabled"
                time.sleep(2)
                ser.write(b'd')  # Envía el carácter 'd'
                print(f"Enviando {file_path} a {serial_port} a {baud_rate} baudios...")
                # Abre el archivo binario en modo lectura
                with open(file_path, "rb") as file:
                    while byte := file.read(1):  # Lee el archivo byte a byte
                        time.sleep(0.0005)
                        ser.write(byte)  # Envía el byte por el puerto serie
                        contador = contador +1
                    
                                            
                finish_time = time.time()
                total_time = finish_time - start_time
                            
                min =  math.trunc(total_time/60)
                sec = round((total_time - (min*60)),)
                messagebox.showinfo('DUMP DONE', "The SRAM has been succesfully writed \n in "+str(min)+" minutes and "+str(sec)+" seconds")
                etiqueta2.config(text=f"No info obtained from cart")                                              # label error
                etiqueta2.config(fg="red")
                boton_enviar["state"] = "active"
                print("Transmisión completada.")
                print(contador)
        except Exception as e:
            print(f"Error: {e}")
        break



# --- INTERFAZ GRÁFICA ---
ventana = tk.Tk()
ventana.geometry("500x200")
ventana.title("SNES DUMP ROM v.0.1.4")
ventana.iconbitmap("icon.ico")
ventana.columnconfigure(1, weight=1)
ventana.columnconfigure(2, weight=1)
ventana.columnconfigure(3, weight=1)
ventana.rowconfigure(1, weight=5)
ventana.rowconfigure(2, weight=1)
ventana.rowconfigure(3, weight=5)
ventana.rowconfigure(4, weight=5)

boton_enviar = tk.Button(ventana, text="Get cart info", command=cart_info)
boton_enviar.grid(row=1, column=1)

boton_iniciar = tk.Button(ventana, text="Start dumping ROM", command=start_dump)
boton_iniciar["state"] = "disabled"
boton_iniciar.grid(row=1, column=2)

boton_iniciar_sram = tk.Button(ventana, text="Start dumping SRAM", command=start_dump_sram)
boton_iniciar_sram["state"] = "disabled"
boton_iniciar_sram.grid(row=2, column=2)

boton_enviar_sram = tk.Button(ventana, text="Start writing SRAM", command=send_sram)
boton_enviar_sram["state"] = "disabled"
boton_enviar_sram.grid(row=1, column=3)

etiqueta = tk.Label(ventana, text="", font=("Meiryo", 10),justify="left") 
etiqueta.grid(row=3, column=2, columnspan=1)

etiqueta2 = tk.Label(ventana, text="", font=("Meiryo", 10), fg="red", justify="left") 
etiqueta2.grid(row=4, column=2, columnspan=1)    


time.sleep(1)
detectar_puerto()
nombre_archivo = tk.StringVar()


ventana.mainloop()

