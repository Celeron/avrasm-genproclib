.IFNDEF	_GENPROCLIB__INCLUDED_
.EQU	_GENPROCLIB__INCLUDED_ = 1
.MESSAGE "Note: <genproclib.inc> (ver.1.0 beta) have included, only once!" 
.include "macrobaselib.inc"	; Библиотека базовых Макроопределений (требуется)
;=== BEGIN "genproclib.inc" ================================================

; Библиотека Подпрограмм общего назначения


;---------------------------------------------------------------------------

; Внимание! В отличие от кода Макросов, Код Подпрограмм из данной библиотеки - 
; всегда и полностью, ВКЛЮЧАЕТСЯ В СЕГМЕНТ КОДА программы - т.е. занимает место! 
; Поэтому, в данном файле должны быть включены только те процедуры, которые реально используются!
; (Неиспользуемый код следует /*...закомментировать...*/, или исключить директивами условной компиляции.)
; Поэтому, ниже, включите определения только для нужных вам секций кода:

.EQU	_GENPROCLIB__EEPROM_	= 1	; Процедуры Записи/Чтения памяти EEPROM
.EQU	_GENPROCLIB__DELAY_	= 1	; Процедуры Временной Задержки
.EQU	_GENPROCLIB__RANDOM_	= 1	; Генератор псевдослучайных чисел (ГПСЧ)
.EQU	_GENPROCLIB__TIMER0_	= 1	; Сервисные процедуры для Timer/Counter0


;---------------------------------------------------------------------------

; Рекомендации по использованию "индексных регистров" в коде:
; 	* РОН из диапазона R26..R31 лучше не использовать для иных нужд, чем как "индексные регистры" (т.е. не рекомендуется использовать их в качестве "временных переменных").
; 	При реализации алгоритма функции, в коде, часто удобно использовать инструкции "непрямой пересылки данных": LD/ST, LDD/STD.
; 	Возникает вопрос выбора: какие при этом лучше использовать "индексные регистры": X, Y, или Z ?
; 	* Регистр Z лучше не использовать для арифметических преобразований, особенно для "параметров функций" - потому что он требуется для инструкций "непрямого перехода" ICALL/IJMP (дальнобойностью до 64K слов), т.е. Z требуется для вызова самой функции. 	(Напомню, что в семействе МК "ATTiny*" нет инструкций "дальнего перехода" CALL/JMP - поэтому, в них, инструкции ICALL/IJMP становятся единственной возможностью!)
; 	* А выбирая между X и Y - в первую очередь, лучше использовать Y, т.к. он более функционален: поддерживает также инструкции LDD/STD. 	(Кроме того, Y - чуть ближе к концу файла РОН, что "концептуальнее": меньше вероятность задействования его, в качестве "временного регистра".)

;---------------------------------------------------------------------------

; Основные положения о "временных переменных" (переменных общего назначения):
;	В коде данной библиотеки, и при работе с ней, регистры R16,R17,R18,R19
;	рекомендуется использовать как "временные переменные" - поэтому, 
;	в вашем прикладном коде, не используйте их для хранения постоянных данных!
; Как сказал DI HALT: 
;	При вычислениях, регистры можно предварительно заталкивать в стек... 
;	но я дам тебе лучше другой совет:
;	когда пишешь программу, продумывай алгоритм так, чтобы использовать регистры 
;	как сплошной TEMP, данные которого актуальны только здесь и сейчас.
;	И что с ними будет в следующей процедуре - уже не важно.
;	А все перманентные данные - следует сохранять в оперативке (SRAM).


; Совет: вы также можете использовать эти псевдонимы регистров в своей программе:
;.def	temp	= R16	; (определено в macrobaselib.inc)
;.def	temp1	= R16	; (определено в macrobaselib.inc)
;.def	temp2	= R17	; (определено в macrobaselib.inc)
;.def	temp3	= R18	; (определено в macrobaselib.inc)
;.def	temp4	= R19	; пока не используется данной библиотекой

;---------------------------------------------------------------------------



.IFDEF	_GENPROCLIB__EEPROM_
;***************************************************************************
;*
;*  Запись / Чтение памяти EEPROM
;*
;***************************************************************************


	; Одни и те же вещи, под разными именами, в разных микроконтроллерах:
	.ifdef	EEPE
		.equ	EepromWriteEnableBit = EEPE;
	.else
	.ifdef	EEWE
		.equ	EepromWriteEnableBit = EEWE;
	.endif
	.endif

	.ifdef	EEMPE
		.equ	EepromMasterWriteEnableBit = EEMPE;
	.else
	.ifdef	EEMWE
		.equ	EepromMasterWriteEnableBit = EEMWE;
	.endif
	.endif

	.ifndef	EEARL
	.ifdef	EEAR
		.equ	EEARL = EEAR			; для самых младших МК
	.endif
	.endif


;---------------------------------------------------------------------------
;
; Записать байт данных в память EEPROM 
; (Tested on: ATMega16, ATTiny25/45/85)
;
; Описание: см. http://easyelectronics.ru/avr-uchebnyj-kurs-ispolzovanie-eeprom.html
; Пример вызова:
;	LDI 	EepromAddressLow,  0			; Загружаем адрес нулевой ячейки EEPROM
;	LDI 	EepromAddressHigh, 0			;  
;	LDI 	EepromDataByte,    45			; и хотим записать в нее число 45
;	RCALL 	EEPROM_WRITE 				; вызываем процедуру записи.
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

.def	EepromDataByte		= R22			; (входной, не меняется)
.def	EepromAddressLow 	= R23			; (входной, не меняется)
.def	EepromAddressHigh 	= R24			; (входной, не меняется)

; Памятка: также использует/портит содержимое регистра TEMP.

;----- Code 

EEPROM_WRITE:	
		SBIC	EECR,	EepromWriteEnableBit	; Ждем готовности памяти к записи:
		RJMP	EEPROM_WRITE			; крутимся в цикле до тех пор, пока не очистится флаг...
 
		;=== начало критической секции
		INR	temp,	SREG			; Save global interrupt flag
		CLI					; запрещаем прерывания

	.ifdef	EEPM0
	.ifdef	EEPM1
		; Во многих моделях требуется ещё сконфигурировать режим программирования EEPROM:
		CBI	EECR,	EEPM0			; Set Programming mode = "Atomic Byte Programming" (erase + write)
		CBI	EECR,	EEPM1			; (Note: If the EEPMn bits are zero, writing EEPE (within four cycles after EEMPE is written) will trigger the erase/write operation. Both the erase and write cycle are done...)
	.endif
	.endif

		OUT 	EEARL,	EepromAddressLow 	; Загружаем адрес нужной ячейки (младший байт)
	.ifdef	EEARH
		OUT 	EEARH,	EepromAddressHigh 	; Загружаем адрес нужной ячейки (старший байт)
	.endif
		OUT 	EEDR,	EepromDataByte 		; и сами данные, которые нам нужно записать

		SBI 	EECR,	EepromMasterWriteEnableBit	; взводим предохранитель
		SBI 	EECR,	EepromWriteEnableBit		; стартуем запись байта	(Note: bit remains set until the erase and write opera-tions are completed)
 
		OUTR	SREG,	temp			; Restore global interrupt flag
		;=== конец критической секции

		RET 					; возврат из процедуры



;---------------------------------------------------------------------------
;
; Прочитать байт данных из памяти EEPROM
; (Tested on: ATMega16, ATTiny25/45/85)
;
; Описание: см. http://easyelectronics.ru/avr-uchebnyj-kurs-ispolzovanie-eeprom.html
; Пример вызова:
;	LDI 	EepromAddressLow,0			; Загружаем адрес нулевой ячейки
;	LDI 	EepromAddressHigh,0			; EEPROM из которой хотим прочитать байт
;	RCALL 	EEPROM_READ				; вызываем процедуру чтения. После которой 
;							; в EepromDataByte будет считанный байт.
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	EepromDataByte		= см.выше		; (выходной)
;.def	EepromAddressLow 	= см.выше		; (входной, не меняется)
;.def	EepromAddressHigh 	= см.выше		; (входной, не меняется)

;----- Code

EEPROM_READ:	
		SBIC 	EECR,	EepromWriteEnableBit	; Ждем пока будет завершена прошлая запись:
		RJMP	EEPROM_READ			; крутимся в цикле до тех пор, пока не очистится флаг...

		OUT 	EEARL,	EepromAddressLow 	; Загружаем адрес нужной ячейки (младший байт)
	.ifdef	EEARH
		OUT 	EEARH,	EepromAddressHigh 	; Загружаем адрес нужной ячейки (старший байт)
	.endif
		SBI 	EECR,	EERE 			; стартуем чтение байта
		IN 	EepromDataByte, EEDR 		; Забираем из регистра данных результат

		RET



;---------------------------------------------------------------------------
;
; Агрегатные процедуры, которые позволяют перекачать сразу массив байтов:
;
; EEPROM_WRITE_SEQUENCE (from SRAM to EEPROM) - записать массив байт из ОЗУ в энергонезависимую память EEPROM.
; EEPROM_READ_SEQUENCE  (from EEPROM to SRAM) - прочитать массив байт из энергонезависимой памяти EEPROM в ОЗУ.
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	EepromAddressLow 	= см.выше		; Начало массива в EEPROM-памяти
;.def	EepromAddressHigh 	= см.выше		;
.def	SramAddressLow 		= R28	; YL		; Начало массива в SRAM-памяти
.def	SramAddressHigh 	= R29	; YH		; 
.def	PumpBytesCount		= R25			; Количество байт, которое скопировать (размер массива)

; Памятка: также использует/портит содержимое регистров TEMP и EepromDataByte (опосредованно).

;----- Code 

EEPROM_WRITE_SEQUENCE:
		TST	PumpBytesCount			; Проверка на дурака: если счётчик уже нулевой?
		BREQ	EXIT__EEPROM_WRITE_SEQUENCE

	LOOP__EEPROM_WRITE_SEQUENCE:
		LD	EepromDataByte,	Y+		; Читаем один байт из SRAM и Инкрементируем адрес SramAddress*
		RCALL	EEPROM_WRITE			; Сохраняем его в EEPROM, по адресу EepromAddress*
		SUBI	EepromAddressLow, (-1)		; Инкрементируем также адрес EepromAddress*
		SBCI	EepromAddressHigh,(-1)
		DEC	PumpBytesCount			; Декрементируем счётчик "количества байт, которое скопировать"
		BRNE	LOOP__EEPROM_WRITE_SEQUENCE	; продолжать, пока не станет ноль в счётчике
	EXIT__EEPROM_WRITE_SEQUENCE:
		RET


;----- Code 

EEPROM_READ_SEQUENCE:
		TST	PumpBytesCount			; Проверка на дурака: если счётчик уже нулевой?
		BREQ	EXIT__EEPROM_READ_SEQUENCE

	LOOP__EEPROM_READ_SEQUENCE:
		RCALL	EEPROM_READ			; Читаем один байт из EEPROM, по адресу EepromAddress*
		ST	Y+,	EepromDataByte		; Сохраняем его в SRAM и Инкрементируем адрес SramAddress*
		SUBI	EepromAddressLow, (-1)		; Инкрементируем также адрес EepromAddress*
		SBCI	EepromAddressHigh,(-1)
		DEC	PumpBytesCount			; Декрементируем счётчик "количества байт, которое скопировать"
		BRNE	LOOP__EEPROM_READ_SEQUENCE	; продолжать, пока не станет ноль в счётчике
	EXIT__EEPROM_READ_SEQUENCE:
		RET



.ENDIF

.IFDEF	_GENPROCLIB__DELAY_
;***************************************************************************
;*
;*  Временная Задержка
;*
;***************************************************************************


;---------------------------------------------------------------------------
;
; Процедура "тупой задержки" (крутит цикл в CPU)
;
; Величина задержки задаётся 3х байтовым ЧИСЛОм:
;	Через регистровые параметры: TEMP1, TEMP2, TEMP3.
;	Или, как вариант, фиксирована константно, через макроопределения.
;
; Итоговая задержка: 
; 	Количество тактов CPU, которое тупить = (ЧИСЛО+1) * 5
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Величина задержки задаётся через регистровые параметры:
;.def 	TEMP1	; = LowByte
;.def	TEMP2	; = MedByte
;.def	TEMP3	; = HighByte


; Если величина задержки задаётся константно, через макроопределения,
; (в программе всегда используется только некоторая фиксированая задержка?)
; тогда раскомментировать определение и задать константу:
;	.EQU	FORCE_LOAD_CONSTANT_DELAY = 1 
;	.equ 	LowByte  = 255
;	.equ	MedByte  = 255
;	.equ	HighByte = 1

;----- Code 


CPU_DUMB_DELAY:

	.ifdef FORCE_LOAD_CONSTANT_DELAY
		LDI	temp1,	LowByte			; Грузим три байта
		LDI	temp2,	MedByte			; Нашей выдержки
		LDI	temp3,	HighByte
	.endif

	LOOP__CPU_DUMB_DELAY:
		SUBI	temp1,	1			; Вычитаем 1			(1 такт CPU)
		SBCI	temp2,	0			; Вычитаем только С		(1 такт CPU)
		SBCI	temp3,	0			; Вычитаем только С		(1 такт CPU)
		BRCC	LOOP__CPU_DUMB_DELAY		; Если нет переноса - переход	(2 такта CPU)

		RET



.ENDIF

.IFDEF	_GENPROCLIB__RANDOM_
;***************************************************************************
;*
;*  Генератор псевдослучайных чисел (ГПСЧ)
;*
;***************************************************************************


;---------------------------------------------------------------------------
;
; Простейший  Генератор псевдослучайных чисел  (байтовых последовательностей)
; использует алгоритм:	RNDii = (RNDi * 17 + 53) {MOD 256}
;
;
; Порядок использования:
; 	1) присвоить "затравку" регистру RandomNumber.
; 	2) CALL RANDOMIZE
; 	3) прочитать очередное псевдостучайное число из регистра RandomNumber.
; 	4) CALL RANDOMIZE
; 	5) прочитать очередное псевдостучайное число из регистра RandomNumber.
; 	6) ...
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

.def	RandomNumber	= R25	; Число генерируемое датчиком случаных чисел

; Памятка: также использует/портит содержимое регистра TEMP.

;----- Code


RANDOMIZE:
		; RandomNumber *= 17		(оптимальная реализация умножения)
		MOV	temp,	RandomNumber	; x1
		LSL	temp			; x2
		LSL	temp			; x4
		LSL	temp			; x8
		LSL	temp			; x16
		ADD	RandomNumber,	temp	; x(16+1) === 0b00010001

		; RandomNumber += 53
		SUBI	RandomNumber,	(-53)	; трюк: -(-53) = +53
						
		; в итоге: RandomNumber = (RandomNumber * 17 + 53) % 256
		RET



.ENDIF

.IFDEF	_GENPROCLIB__TIMER0_
;***************************************************************************
;*
;*  Сервисные процедуры для Timer/Counter0
;*
;***************************************************************************


	; Одни и те же вещи, под разными именами, в разных микроконтроллерах:
	.ifndef	PSRSYNC
	.ifdef	PSR10
		.equ	PSRSYNC = PSR10
	.else
	.ifdef	PSR0
		.equ	PSRSYNC = PSR0
	.endif
	.endif
	.endif


;---------------------------------------------------------------------------
;
; Процедура: "Сбросить Timer/Counter0 (счётчик и предделитель)"
; (Tested on: ATTiny25/45/85)
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: также использует/портит содержимое регистра TEMP.

;----- Code

TIMER0_RESET:

		;=== Сбросить счётный регистр:

	.ifndef	TCNT0L
		; В случае 8-разрядного Timer/Counter0
		OUTI	TCNT0,	0			; начальное значение счётчика
	.else
		; В случае 16-разрядного Timer/Counter0
		; Note: To do a 16-bit write, the high byte must be written before the low byte. For a 16-bit read, the low byte must be read before the high byte.
		; 	It is important to notice that accessing 16-bit registers are atomic operations. Aware interrupts, use cli/sei!
		OUTI	TCNT0H,	High(0)
		OUTI	TCNT0L,	Low (0)			; начальное значение счётчика
	.endif


		;=== Сбросить Предделитель:

	.ifdef	TSM
		SETB	GTCCR,	TSM			; Synchronization Mode ON
	.endif

		SETB	GTCCR,	PSRSYNC			; Prescaler Reset Request

	.ifdef	TSM
		CLRB	GTCCR,	TSM			; Synchronization Mode OFF and GO reset!	(Note: When the TSM bit is written to zero, the PSR0 bit is cleared by hardware, and the timer/counter start counting.)
	.endif

		RET



.ENDIF

;=== END "genproclib.inc" ==================================================
; coded by (c) Celeron, 2014 @ http://we.easyelectronics.ru/my/Celeron/
.ENDIF
