# PROYECTO FINAL
# ROBOT MOVIL

1. [Descripción]

    El presente proyecto corresponde a la implementación de un programa que comanda manualmente, mediante transmisión por BLE, un robot móvil diferencial el cual de manera automática evade obstáculos que se encuentran a 12 cm de distancia por medio de un sensor de ultrasonido. Para esto, se utilizó un control PI para los motores (con encoders de canales A y B). Además, se anuncia mediante un led RGB el tipo de actividad que realiza tornando un color diferente cuando se encuentra estasionado, en movimiento o evadiendo un obstáculo.

    El programa esta compuesto por los siguientes componentes:

    ble: corresponde a la conexión y envío de datos mediante BLE entre el ESP32 y el dispositivo móvil.

    cmotor: se refiere al control de los motores, mediante un control PI, por ciclos de 50ms (SAMPLE_TIME_MS), pone a 0% e duty cuando no recibe comandos a 100ms (TIMEOUT_MS).

    global: declara la variable global 'comando'. 

    rgbled: se encarga del cambio de color del led rgb del pin 48 del ESP32.

    ssonido: realiza el cálculo de la distancia que se encuentra el sensor de ultrasonido a cualquier obstáculo. Indicará un error si la distancia es mayor a 5 m.

    
2. [Pruebas]

    Las pruebas finales que se realizaron fueron en vacío (patas arriba) y bajo tracción (sobre superficie).
    Durante las pruebas en vacío, el cambio de dirección del giro de los motores presentaba cierta oscilación que no pudo ser corregida, se cree que puede haberse debido a la poca linealibilidad que podrían presentar.

    La oscilación presente en la prueba en vacío no fue apreciable durante las pruebas finales, pero lo más notable es que los motores sólo podían mover el robot a velocidades relativamente altas, lo cual afectaba negativamente la respuesta ante obstáculos. La evasión de obstáculos funcionaba mejor cuando el robot se movía a una media o baja, para esto, se envíaban pocos pulsos desde el comando (app en el dispositivo móvil).

3. [Métricas]

    Las siguiente métricas pudieron ser capturadas cuando el robot se encontraba estacionado:
    
        Task            Time(us)        %CPU
        control_task    2846234         1%
        task_monitor    10459774                6%
        IDLE1           154079292               96%
        IDLE0           145317800               90%
        rgbled_task     96649           <1%
        hcsr04_task     7125951         4%
        hciT            5841            <1%
        BTU_TASK        7118            <1%
        esp_timer       17153           <1%
        btController    220303          <1%
        BTC_TASK        2675            <1%
        ipc0            48547           <1%
        ipc1            62961           <1%
    
    De la anterior lectura se puede observar que por el procentaje de los IDLEs, la mayor parte del tiempo el CPU se mantiene ocioso. Sin embargo, entre las tareas, se puede observar (al margen de task_monitor) que hcsr04_task consume un 4%, la mayor de todas las tareas, esto puede deberse a que dicha tarea usa una función llamada ets_delay_us() la cual que mantiene ocupado al CPU varios microsegundos en un while.

4. [COMENTARIOS]

    Es posible realizar mejoras sobre el programa actual. Por ej. en lugar de poner 0% el duty cuando encuentra un obstáculo, se puede invertir la dirección de giro del motor de manera escalonada (sino corre el riesgo que se voltee si el frenado es en seco) para que genere un frenado paulatino a una distancia mayor que la mínima configurada sólo cuando la velocidad sea alta y la colisión inminente, además, llamar esta evasión por medio de una interrupción. Asimismo, puede agregarse filtros al control PI, entre otras herramientas que puedan mantener más estable los motores.
