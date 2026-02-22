import subprocess
import threading
import serial
import os
import psutil
import tkinter as tk
import time
from tkinter import messagebox
from tkinter import filedialog
from tkinter import ttk
import serial
import serial.tools.list_ports


baudios = 250000
emulador_proceso= None

directorio_internal = os.path.dirname(os.path.abspath(__file__))
directorio_actual = os.path.dirname(directorio_internal)
#print("Directorio actual:", directorio_actual)
#print("Directorio superior:", directorio_internal)

database = os.path.join(directorio_actual,'DB.txt')

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

# --- Comprobación y lectura del archivo de configuración ---
config_path = os.path.join(directorio_actual, 'config.txt')

# Si no existe config.txt, lo crea con un par de líneas de ejemplo
if not os.path.exists(config_path):
    with open(config_path, 'w') as archivo_config:
        archivo_config.write(
            "# Archivo de configuración del lector de cartuchos SNES\n"
            "# Usa la siguiente sintaxis para indicar la ruta del emulador:\n"
            "# emulador \"C:\\Ruta\\al\\emulador\\snes9x.exe -fullscreen\"\n\n"
            "emulador \"C:\\Users\\Pedro\\Dropbox\\python proyectos\\Proyecto C\\Software windows\\dist\\snes9x-1.63-win32\\snes9x.exe -fullscreen\"\n"
            "roms \"C:\\roms\"\n"
        )
    print(f"⚠️  No se encontró 'config.txt'. Se ha creado un archivo de ejemplo en:\n{config_path}")
    messagebox.showinfo("Error",f"⚠️  No se encontró 'config.txt'. Se ha creado un archivo de ejemplo en:\n{config_path}")

else:
    print(f"✅ Archivo de configuración encontrado: {config_path}")

# Ahora sí, lee el archivo de configuración
with open(config_path, 'r') as archivo_config:
    lineas = archivo_config.readlines()



# Busca la línea que contiene la palabra "emulador" en el archivo config y guardo en ruta_al_exe la ruta al emulador
for linea in lineas:
    linea = linea.strip()
    # Ignorar líneas vacías o que comiencen con '#'
    if not linea or linea.startswith("#"):
        continue

    if 'emulador' in linea:
        # Divide la línea en palabras
        palabras = linea.split()
        if len(palabras) >= 2:
            # La ruta puede estar entre comillas, así que la reconstruimos
            ruta_al_exe = ' '.join(palabras[1:]).strip('"')
            break
else:
    print("No se encontró la configuración del emulador en el archivo.")
    exit(1)


#REPETIMOS PARA BUSCAR AHORA LA LINEA DONDE SE INDICA LA CARPETA DE LAS ROMS
with open(config_path, 'r') as archivo_config:
    lineas = archivo_config.readlines()

# Busca la línea que contiene la palabra "roms" en el archivo config y guardo en ruta_al_roms 
for linea in lineas:
    linea = linea.strip()
    # Ignorar líneas vacías o que comiencen con '#'
    if not linea or linea.startswith("#"):
        continue

    if linea.startswith("roms"):
        # Divide la línea en palabras
        palabras = linea.split()
        if len(palabras) >= 2:
            # La ruta puede estar entre comillas, así que la reconstruimos
            ruta_al_roms = ' '.join(palabras[1:]).strip('"')
            #print(ruta_al_roms)
            break
else:
    print("No se encontró la configuración del emulador en el archivo.")
    exit(1)


def get_file_name(game_code): #esta funcion recibe el codigo que manda el arduino, comprueba la base de datos y devuelve el nombre del archivo que tiene que abrir el emulador.
    if not os.path.exists(database):
        print("⚠️  ERROR: No se encontró el archivo DB.txt en esa ruta.")
        messagebox.showinfo("Error","⚠️  ERROR: No se encontró el archivo DB.txt. Cree uno primero pulsando el boton MAKE BD")
        return None
    else:
        print("✅ Archivo DB.txt encontrado correctamente.")

    
    
    
    
    with open(database, 'r') as file:
        for line in file:
            #nombre, contenido = line.strip().split("; ")
            parts = line.strip().split("; ")
            #print(f"Para la dirección: {contenido}, el nombre del archivo es: {nombre}")
            game_code = game_code.strip().lower() #lo ponemos en minusculas, para comparar minúsculas con minúsculas
            #print(f"Comparando repr: {repr(parts[1].lower())}  vs  {repr(game_code)}")
            
            if len(parts) >= 2 and parts[1].lower() == game_code:
                nombre = parts[0].strip()
                launch_game(nombre)
                return nombre
            


def launch_game(file_name):
    global emulador_proceso #vamos a usar esta variable para guardar el proceso del emulador para poder cerrarlo.
    # Intenta ejecutar el archivo exe
    try:
        print(ruta_al_exe + " "+ "\""+directorio_actual+"\\roms\\"+file_name+"\"" )
        emulador_proceso =subprocess.Popen(ruta_al_exe + " "+ "\""+ruta_al_roms+"\\"+file_name+"\"" )
        
    except FileNotFoundError:
        print(f"No se pudo encontrar el archivo {ruta_al_exe}")
        messagebox.showinfo("Error", f"No se pudo encontrar el archivo {ruta_al_exe}")
    except Exception as e:
        print(f"Ocurrió un error al ejecutar el archivo: {str(e)}")

def cerrar_aplicacion(nombre_aplicacion):
    global emulador_proceso
    if emulador_proceso is not None:
        try:
            print(f"Cerrando emulador (PID {emulador_proceso.pid})...")
            emulador_proceso.terminate()
            emulador_proceso.wait(timeout=3)
            print("Emulador cerrado correctamente.")
        except psutil.TimeoutExpired:
            print("El emulador no respondió, forzando cierre...")
            emulador_proceso.kill()
        except Exception as e:
            print(f"Error al cerrar el emulador: {e}")
        finally:
            emulador_proceso = None  # Limpieza
    else:
        print("No hay emulador en ejecución.")


# Configura el puerto serie
#puerto_serie = serial.Serial(puerto_ch340, baudios)  # Reemplaza 'COM1' con el nombre de tu puerto serie y la velocidad correcta
def escuchar_puerto():
    puerto_ch340= detectar_puerto()
    #print (baudios)
    #print (puerto_ch340)
    puerto_serie = serial.Serial(puerto_ch340, baudios)
    time.sleep(2)
    puerto_serie.write(b'e')  # Envía el carácter 'e'
    print("Carácter 'e' enviado")
    try:
        while True:
            # Lee la palabra recibida por el puerto serie
            palabra_recibida = puerto_serie.readline().decode().strip().lower()
            # Imprime la palabra en la consola
            if palabra_recibida == "off":
                cerrar_aplicacion("snes9x.exe")
            else:
                print("Palabra recibida:", palabra_recibida)
                print("nombre del archivo:", get_file_name(palabra_recibida))

        

    except KeyboardInterrupt:
        print("Se interrumpió la recepción de datos por el puerto serie.")
    finally:
        puerto_serie.close()

def obtener_contenido_hexadecimal(archivo):
    contenido = ''
    with open(archivo, 'rb') as file:
        file.seek(0x7fdc)               
        checksum = int.from_bytes(file.read(2))
        #print ("checksum", checksum)
        file.seek(0x7fde)
        checksumcompli= int.from_bytes(file.read(2))
        #print ("checksumcompli", ~checksumcompli & 0xFFFF)
        if checksum == ~checksumcompli & 0xFFFF: #compruebo el chechsum y el complemente en las direcciones para lorom, si se verifica es lorom sino hirom
             direcciones = [0x7FC0, 0x7FDC]      # LOROM 
        else:
             direcciones = [0xFFC0, 0xFFDC]   #HIROM 

        for direccion in direcciones:
                file.seek(direccion)  # Ir a la dirección de memoria específica
                contenido += file.read(4).hex().upper()  # Leer 4 bytes en esa dirección y concatenar en hexadecimal
    return contenido

def escribir_archivo_texto(directorio, archivo_texto):
    # Verificar si el directorio existe
    if not os.path.exists(directorio):
        messagebox.showerror("Error", f"The directory was not found:\n{directorio}\n Change it in config.txt file")
        return  # salir de la función sin hacer nada más
    
    # Verificar si hay archivos dentro del directorio
    archivos = os.listdir(directorio)
    if not archivos:
        messagebox.showerror("Error", f"No files found in directory:\n{directorio}")
        return

    try:
        with open(archivo_texto, 'w') as file:
            for archivo in archivos:
                archivo_path = os.path.join(directorio, archivo)
                contenido_hexadecimal = obtener_contenido_hexadecimal(archivo_path)
                file.write(f"{archivo}; {contenido_hexadecimal}\n")
        messagebox.showinfo("DATABASE DONE", "The DataBase has been successfully created")
    except Exception as e:
        messagebox.showerror("Error", f"An error occurred while writing the database:\n{str(e)}")
           

# Directorio con las roms
#directorio = directorio_actual+"/roms"

# Archivo de texto que se generará la base de datos que asocia cada rom con su código
archivo_texto = directorio_actual+"/"+"DB.txt"

# --- INTERFAZ GRÁFICA ---
ventana = tk.Tk()
ventana.geometry("500x200")
ventana.title("SUPER NINTENDO CART PLAYER")
ventana.iconbitmap("icon.ico")

# Configuración del grid: 3 columnas y 4 filas para centrar mejor
for i in range(3):
    ventana.columnconfigure(i, weight=1)
for i in range(4):
    ventana.rowconfigure(i, weight=1)

# Etiqueta superior (mensaje principal)
etiqueta2 = tk.Label(
    ventana,
    text="Put your Snes cart on and push up the button, or...",
    font=("Meiryo", 10),
    fg="red",
    justify="center"
)
etiqueta2.grid(row=1, column=1, sticky="n", pady=10)

# Etiqueta inferior (puedes usarla para mensajes dinámicos)
etiqueta = tk.Label(
    ventana,
    text="",
    font=("Meiryo", 10),
    justify="center"
)
etiqueta.grid(row=3, column=1, pady=5)

# Botón centrado debajo de las etiquetas
boton_BD = tk.Button(
    ventana,
    text="Make DB",
    command=lambda: escribir_archivo_texto(ruta_al_roms, archivo_texto),
    width=15,
    height=1
)
boton_BD.grid(row=2, column=1, pady=10)

# Lanzar hilo y bucle principal
time.sleep(1)
hilo_puerto = threading.Thread(target=escuchar_puerto, daemon=True)
hilo_puerto.start()

ventana.mainloop()

