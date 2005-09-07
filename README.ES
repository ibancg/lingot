LINGOT - Afinador de instrumentos musicales

Escrito por:
  Ibán Cereijo Graña <ibancg@gmail.com>
  Jairo Chapela Martínez <jairocm@wanadoo.es>

Copyright (C) 2004, 2005 Ibán Cereijo Graña, Jairo Chapela martínez,
		   Graciano Carrillo Pousa.

Descripción
===========

	Lingot es un afinador de instrumentos musicales, preciso, fácil de usar
y altamente configurable. Originalmente concebido para afinar guitarras eléc-
tricas, su configurabilidad le da un carácter más general (la afinación de otro
tipo de instrumentos todavía no ha sido testada).

	Simula el aspecto de un afinador analógico con una aguja que indica la
desviación respecto a una nota dada, encontrada automáticamente como la nota 
más cercana a la frecuencia estimada, indicando dicha nota y frecuencia.

	La nota será encontrada de forma automática, no teniendo un modo de
funcionamiento manual (indicar manualmente la nota que queremos afinar), para
mantener la generalidad.

	Recomendamos usar el afinador conjuntamente con un mezclador para 
seleccionar la fuente de grabación y los niveles de señal.

Requisitos
==========

	* Tarjeta de sonido.
	* Kernel linux con soporte para audio (OSS).
	* librería GTK+ 2.0.
	
Instalación
===========

	Consulta el archivo INSTALL

Parámetros de entrada
=====================

	lingot [-c configuracion]

busca en el directorio ~/.lingot/ un archivo llamado {configuracion}.conf

por ejemplo:

	lingot -c bass

Tomará la configuración del archivo ~/.lingot/bass.conf, esto es útil para
mantener varias configuraciones correspondientes a varios instrumentos.

Archivo de configuración
========================

	Al lanzarse por primera vez se creará el archivo de configuración:
~/.lingot/lingot.conf

Las opciones se pueden cambiar también en el GUI, y recomendamos cambiarlas
desde esta interfaz. Sus valores por defecto están optimizados para afinar
guitarra eléctrica, para otros instrumentos debe ser retocado (es buena idea
guardar un archivo de configuración por cada instrumento, y utilizar la 
opción -c para cargar una configuración u otra).

Con la finalidad de mantener la máxima configurabilidad, el usuario dispone de
multitud de opciones, y la comprensión total del efecto de las mismas requiere 
conocimientos de procesado de señal, a continuación se intentará explicar el
efecto de cada una, pero si no se comprende el resultado lo mejor es no 
cambiarlas. No se descarta en futuras versiones reducir el número de opciones
para hacer el afinador más manejable.


Describimos las opciones que encontraremos en este archivo, y recomendamos su
manipulación en función de la precisión de afinado obtenida y la potencia de
cálculo requerida.

AUDIO_DEV (dispositivo de sonido)

	 Dispositivo de audio seleccionado.

SAMPLE_RATE (frecuencia de muestreo)

	 Frecuencia a la que será muestreada la señal de entrada por la tarjeta
	 de sonido. No todas las tarjetas soportan cualquier valor para esta 
	 opción, y recomendamos escoger una de las opciones que proporciona
	 el GUI:

	  8000 Hz
	 11025 Hz
	 22050 Hz
	 44100 Hz

	 Podremos encontrar una frecuencia efectiva de muestreo submúltiplo de 
	 la que pongamos aquí mediante el factor de sobremuestreo.

	 Para seleccionar una frecuencia de muestreo debemos tener en cuenta la
	 frecuencia máxima que queramos afinar en nuestro instrumento, y poner
	 la frecuencia efectiva de muestreo (ya dividida por el factor de 
	 sobremuestreo) al menos al doble de esa frecuencia (criterio de 
	 Nyquist). Todas las componentes frecuenciales por encima de la mitad
	 de la frecuencia de muestreo efectiva serán filtradas y no aparecerán
	 en el espectro.

	 Se expresa como un numero entero, en herzios.

OVERSAMPLING (factor de sobremuestreo)

 	 Indica el factor por el que se va a diezmar la señal muestreada, 
	 consiguiendo una frecuencia de muestreo efectiva equivalente a la de
	 muestreo real de la tarjeta de sonido dividido por este factor.

	 Por ejemplo, para conseguir una frecuencia de muestreo efectiva de 
         4 KHz (podemos afinar tonos de hasta 2 KHz) pondremos una frecuencia 
	 de muestreo de 8 KHz y un factor de sobremuestreo de 2.

	 Debe ser un numero entero. Adimensional.

ROOT_FREQUENCY_ERROR (error de la nota de referencia LA)

	 Cuando queremos afinar con un cierto error de desplazamiento en
	 frecuencia. Esto es útil cuando queremos adaptar la afinación del
	 instrumento a la de una grabación que tiene una afinación desplazada.

	 Por ejemplo, si escuchamos que un LA en una grabación se encuentra
	 un cuarto de tono por encima del que debería ser (440 Hz), pondremos
	 un error del 50% (el error se expresa en porcentaje 
	 de semitonos). 

	 Se expresa como un número real en tanto por uno (0.5 expresa un error
	 del 50%). 
	 
FFT_SIZE (tamaño FFT)

	 Un valor elevado proporciona mayor precisión, sobre todo mayor 
	 precisión de enganche con la frecuencia deseada, pero supone mayor
	 potencia de cálculo.

	 Debe tenerse en cuenta antes de subir este valor innecesariamente 
	 el intervalo temporal de cálculo que implica: tener un buffer de 4096
	 muestras para la FFT con una frecuencia de muestreo efectiva de 8 KHz
	 implica que cada transformada necesita valores temporales de los 
	 últimos 512 ms, así pues no tiene sentido poner una ventana temporal
	 menor. Este parámetro influye en la respuesta temporal del afinador.

	 Veamos un ejemplo más extremo:

	 Supongamos que tenemos un instrumento capaz de generar frecuencias 
	 bajas (< 200 Hz), y por tanto ponemos una frecuencia de muestreo de
	 400 Hz (frecuencia de 8 KHz y factor de sobremuestreo de 20). Con el
	 fin de obtener la máxima precisión inicial establecemos 4096 muestras 
	 para el buffer de FFT, esto implicará que la ventana temporal mínima 
	 debe ser 10.24 segundos, es decir, en cada afinación tenemos sonidos 
  	 de hasta hace 10.24 segundos contaminando la afinación, dicho en otras
	 palabras, tendremos que esperar 10.24 segundos entre nota y nota para
	 no interferir.

	 Nótese que el espectro sólo muestra 256 muestras, y para valores altos
	 de este parámetro puede cuadrar la frecuencia fundamental encontrada
	 fuera del campo de visión (de momento no está implementado un visor
	 que se desplace en frecuencia)

	 Debe ser un numero entero potencia de 2.
	 
TEMPORAL_WINDOW (ventana temporal)

	 Indica la cantidad de señal en tiempo que se usa para aproximar la
	 frecuencia final. Cuanto mayor sea este valor, más preciso será el
	 resultado, pero de respuesta más lenta y más contaminada por notas
	 anteriores.

	 Como regla práctica diremos que si tenemos una ventana temporal de 1 
	 segundo, debemos esperar 1 segundo entre nota y nota para obtener la 
	 afinación. Así pues, este parámetro repercute directamente en la 
	 respuesta dinámica del afinador.

	 La ventana temporal en muestras (obtenida como la multilpicación de su
	 duración en segundos y la frecuencia efectiva de muestreo) debe ser
	 mayor que el buffer de FFT.

	 Se expresa como un número real, en segundos.

NOISE_THRESHOLD (umbral de ruido)

 	 Para no afinar ruido establecemos un umbral distinguiendo lo que es 
	 ruido y lo que es señal. Establézcase empíricamente y ayúdese de un
	 mezclador para los niveles de potencia.

	 Es un número real expresado en dB.

CALCULATION_RATE (frecuencia de cálculo)

	 Cuantos más cálculos por segundo mayor sensación dinámica nos dará el
	 afinador, pero necesitará mayor potencia de cálculo.

VISUALIZATION_RATE (frecuencia de visualización)

	 Tiene impacto en la sensación de dinamismo conseguida, pero menos en
	 la potencia de cálculo necesaria.

	 No tiene sentido que la frecuencia de cálculo sea mayor que la de 
	 visualización.

PEAK_NUMBER (número de picos)

	 Para la identificación del pico fundamental debemos tener en cuenta,
	 en función de las características del timbre generado por el 
	 instrumento, el número de picos armónicos (en frecuencias múltiplos de
	 la fundamental) que pueden superar en magnitud al pico fundamental 
	 (hasta la relación señalada en la siguiente opción).

PEAK_REJECTION_RELATION (relación de rechazo de picos)

	 Cualquier pico cuya magnitud tenga con la magnitud del pico máximo una
	 relación mayor que la señalada será descartado (características del
	 timbre del instrumento).

	 Es un número real expresado en dB.

PEAK_ORDER (orden de pico)

	 Número de muestras necesarias para considerar un pico como tal. La 
	 correcta afinación de este parámetro dependerá del tipo de timbre del
	 instrumento, y más de la resolución espectral obtenida mediante la
	 frecuencia efectiva de muestreo y el tamaño de FFT, nótese que 
	 considerar N muestras en la FFT implica un intervalo frecuencial 
	 variable dependiendo de aquellos parámetros.

DFT_NUMBER (número de DFTs)

	 Tras encontrar el pico máximo en la FFT se hacen un cierto número de
	 DFTs localizadas en frecuencia para cercar más la frecuencia 
	 aproximada antes de lanzar la aproximación final por método iterativo
	 (Newton-Raphson).

	 Cuantas más DFTs más precisión de enganche tenemos, pero más potencia
	 de cálculo necesitamos.

DFT_SIZE (tamaño de las DFTs)

	 Tamaño en muestras de las DFTs, cuanto mayor sea, más precisión de
	 enganche, pero mayor potencia de cálculo necesaria.


Funcionamiento
==============

	Consulta la documentación técnica.

