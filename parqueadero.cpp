// ============================================================
//  SISTEMA DE ADMINISTRACION DE PARQUEADERO
//  Parcial Corte 2 - Pensamiento Computacional
// ============================================================
//  Librerias permitidas unicamente
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

// ============================================================
//  CONSTANTES DEL MAPA  (cumple: minimo 16x16)
// ============================================================
const int FILAS    = 20;
const int COLUMNAS = 20;

// Simbolos que se usan para dibujar el mapa
const char VACIO   = '.';   // pasillo / via de acceso
const char PARED   = '#';   // borde o isla decorativa
const char LIBRE   = 'O';   // espacio de parqueo disponible
const char OCUPADO = 'X';   // espacio de parqueo ocupado
const char ENTRADA = 'E';   // punto de entrada de vehiculos
const char SALIDA  = 'S';   // punto de salida de vehiculos
const char RUTA    = '*';   // camino trazado temporalmente

// Tarifa base en pesos colombianos por minuto
const double TARIFA_POR_MINUTO = 100.0;

// ============================================================
//  VARIABLES GLOBALES
//  Compartidas por todas las funciones del programa
// ============================================================
char mapa[FILAS][COLUMNAS];

// Los datos de cada vehiculo se guardan en 5 vectores paralelos.
// El vehiculo numero N esta en el indice N de CADA vector.
// Ejemplo: placas[2] y horas_entrada[2] pertenecen al mismo vehiculo.
std::vector<std::string> placas;            // placa de cada vehiculo
std::vector<int>         horas_entrada;     // hora en que entro
std::vector<int>         minutos_entrada;   // minuto en que entro
std::vector<int>         filas_espacio;     // fila del espacio asignado
std::vector<int>         columnas_espacio;  // columna del espacio asignado
std::vector<bool>        activos;           // true si sigue en el parqueadero

// Reloj simulado: empieza a las 8:00 y avanza con cada operacion
int hora_actual   = 8;
int minuto_actual = 0;

// ============================================================
//  FUNCION: color_de_celda  (obligatorio del parcial - lambda)
//  Recibe un simbolo del mapa y devuelve su codigo de color ANSI.
//  Como "auto" no esta permitido, se declara como funcion normal.
// ============================================================
std::string color_de_celda(char celda) {
    if (celda == PARED)    return "\033[90m";  // gris oscuro
    if (celda == LIBRE)    return "\033[32m";  // verde
    if (celda == OCUPADO)  return "\033[31m";  // rojo
    if (celda == ENTRADA)  return "\033[33m";  // amarillo
    if (celda == SALIDA)   return "\033[35m";  // magenta
    if (celda == RUTA)     return "\033[36m";  // cyan
    return "\033[37m";                          // blanco para vias
}

// ============================================================
//  FUNCION: dibujar_barra  (obligatorio del parcial - lambda)
//  Imprime una barra tipo [====----] segun el porcentaje dado.
//  Como "auto" no esta permitido, se declara como funcion normal.
// ============================================================
void dibujar_barra(double porcentaje_actual, int largo_barra) {
    int bloques_llenos = (int)(porcentaje_actual / 100.0 * largo_barra);
    std::cout << "  [";
    for (int posicion = 0; posicion < largo_barra; posicion++) {
        if (posicion < bloques_llenos) {
            std::cout << "\033[31m" << "=" << "\033[0m"; // rojo = ocupado
        } else {
            std::cout << "\033[32m" << "-" << "\033[0m"; // verde = libre
        }
    }
    std::cout << "] " << (int)porcentaje_actual << "%\n\n";
}

// ============================================================
//  REGISTRO EN ARCHIVO  (innovacion: log persistente)
//  Cada ingreso y salida queda guardado en disco
// ============================================================
void guardar_en_historial(const std::string& linea) {
    // ios::app agrega al final sin borrar lo que ya habia
    std::ofstream archivo("historial_parqueadero.txt", std::ios::app);
    if (archivo.is_open()) {
        archivo << linea << "\n";
        archivo.close();
    }
}

// ============================================================
//  CONSTRUIR EL MAPA INICIAL
//  Coloca bordes, vias, espacios de parqueo, entrada y salida
// ============================================================
void construir_mapa() {
    // Paso 1: llenar todo de vias (pasillos por donde circulan los vehiculos)
    for (int fila = 0; fila < FILAS; fila++) {
        for (int columna = 0; columna < COLUMNAS; columna++) {
            mapa[fila][columna] = VACIO;
        }
    }

    // Paso 2: poner paredes en todo el borde del mapa
    for (int fila = 0; fila < FILAS; fila++) {
        mapa[fila][0]          = PARED;
        mapa[fila][COLUMNAS-1] = PARED;
    }
    for (int columna = 0; columna < COLUMNAS; columna++) {
        mapa[0][columna]       = PARED;
        mapa[FILAS-1][columna] = PARED;
    }

    // Paso 3: entrada en columna 2 y salida en columna 17 (ambas en fila 0)
    mapa[0][2]  = ENTRADA;
    mapa[0][17] = SALIDA;

    // Paso 4: bloques de espacios de parqueo distribuidos en el mapa
    // Bloque izquierdo: filas 3,5,7 - columnas 2,3,4
    for (int fila = 3; fila <= 7; fila += 2) {
        for (int columna = 2; columna <= 4; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    // Bloque central-izquierdo: filas 3,5,7 - columnas 7,8,9
    for (int fila = 3; fila <= 7; fila += 2) {
        for (int columna = 7; columna <= 9; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    // Bloque central-derecho: filas 3,5,7 - columnas 11,12,13
    for (int fila = 3; fila <= 7; fila += 2) {
        for (int columna = 11; columna <= 13; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    // Bloque derecho: filas 3,5,7 - columnas 15,16,17
    for (int fila = 3; fila <= 7; fila += 2) {
        for (int columna = 15; columna <= 17; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    // Segunda zona de espacios: filas 10,12,14
    for (int fila = 10; fila <= 14; fila += 2) {
        for (int columna = 2; columna <= 4; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    for (int fila = 10; fila <= 14; fila += 2) {
        for (int columna = 7; columna <= 9; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    for (int fila = 10; fila <= 14; fila += 2) {
        for (int columna = 11; columna <= 13; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }
    for (int fila = 10; fila <= 14; fila += 2) {
        for (int columna = 15; columna <= 17; columna++) {
            mapa[fila][columna] = LIBRE;
        }
    }

    // Paso 5: islas decorativas en la parte baja del mapa
    for (int fila = 17; fila <= 18; fila++) {
        for (int columna = 7; columna <= 13; columna++) {
            mapa[fila][columna] = PARED;
        }
    }
}

// ============================================================
//  MOSTRAR EL MAPA EN CONSOLA con colores
// ============================================================
void mostrar_mapa() {
    std::cout << "\n";
    // Encabezado de numeros de columna
    std::cout << "   ";
    for (int columna = 0; columna < COLUMNAS; columna++) {
        std::cout << (columna % 10);
    }
    std::cout << "\n";

    // Imprimir cada celda con su color usando color_de_celda
    for (int fila = 0; fila < FILAS; fila++) {
        if (fila < 10) std::cout << " " << fila << " ";
        else           std::cout << fila << " ";

        for (int columna = 0; columna < COLUMNAS; columna++) {
            std::cout << color_de_celda(mapa[fila][columna])
                      << mapa[fila][columna]
                      << "\033[0m";
        }
        std::cout << "\n";
    }

    // Leyenda de simbolos del mapa
    std::cout << "\n  LEYENDA: "
              << "\033[32m" << "O" << "\033[0m" << "=Libre  "
              << "\033[31m" << "X" << "\033[0m" << "=Ocupado  "
              << "\033[37m" << "." << "\033[0m" << "=Via  "
              << "\033[90m" << "#" << "\033[0m" << "=Pared  "
              << "\033[33m" << "E" << "\033[0m" << "=Entrada  "
              << "\033[35m" << "S" << "\033[0m" << "=Salida\n\n";
}

// ============================================================
//  CONTAR ESPACIOS LIBRES Y OCUPADOS EN EL MAPA
// ============================================================
int contar_espacios_libres() {
    int total = 0;
    for (int fila = 0; fila < FILAS; fila++) {
        for (int columna = 0; columna < COLUMNAS; columna++) {
            if (mapa[fila][columna] == LIBRE) {
                total++;
            }
        }
    }
    return total;
}

int contar_espacios_ocupados() {
    int total = 0;
    for (int fila = 0; fila < FILAS; fila++) {
        for (int columna = 0; columna < COLUMNAS; columna++) {
            if (mapa[fila][columna] == OCUPADO) {
                total++;
            }
        }
    }
    return total;
}

// ============================================================
//  BUSCAR EL PRIMER ESPACIO LIBRE EN EL MAPA
//  Usa punteros para devolver la posicion encontrada
// ============================================================
bool buscar_primer_espacio_libre(int* fila_encontrada, int* columna_encontrada) {
    for (int fila = 0; fila < FILAS; fila++) {
        for (int columna = 0; columna < COLUMNAS; columna++) {
            if (mapa[fila][columna] == LIBRE) {
                *fila_encontrada    = fila;
                *columna_encontrada = columna;
                return true;
            }
        }
    }
    return false;
}

// ============================================================
//  MOSTRAR RUTA DESDE LA ENTRADA HASTA UN ESPACIO
//  Traza el camino con '*' en una copia del mapa
// ============================================================
void mostrar_ruta_hasta_espacio(int fila_destino, int columna_destino) {
    // Copiar el mapa original para no modificarlo permanentemente
    char mapa_con_ruta[FILAS][COLUMNAS];
    for (int fila = 0; fila < FILAS; fila++) {
        for (int columna = 0; columna < COLUMNAS; columna++) {
            mapa_con_ruta[fila][columna] = mapa[fila][columna];
        }
    }

    // Trazar camino vertical bajando desde fila 1 hasta fila_destino
    for (int fila = 1; fila <= fila_destino; fila++) {
        if (mapa_con_ruta[fila][2] == VACIO) {
            mapa_con_ruta[fila][2] = RUTA;
        }
    }

    // Trazar camino horizontal hasta la columna del espacio
    int inicio = (columna_destino < 2) ? columna_destino : 2;
    int fin    = (columna_destino > 2) ? columna_destino : 2;
    for (int columna = inicio; columna <= fin; columna++) {
        if (mapa_con_ruta[fila_destino][columna] == VACIO) {
            mapa_con_ruta[fila_destino][columna] = RUTA;
        }
    }

    // Imprimir el mapa con la ruta marcada
    std::cout << "\n   RUTA DE ACCESO AL ESPACIO ("
              << fila_destino << "," << columna_destino << ")\n\n";
    std::cout << "   ";
    for (int columna = 0; columna < COLUMNAS; columna++) {
        std::cout << (columna % 10);
    }
    std::cout << "\n";

    for (int fila = 0; fila < FILAS; fila++) {
        if (fila < 10) std::cout << " " << fila << " ";
        else           std::cout << fila << " ";
        for (int columna = 0; columna < COLUMNAS; columna++) {
            std::cout << color_de_celda(mapa_con_ruta[fila][columna])
                      << mapa_con_ruta[fila][columna]
                      << "\033[0m";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ============================================================
//  VALIDAR QUE LA PLACA TENGA EL FORMATO CORRECTO
//  Formato colombiano: 3 letras + 3 numeros  (ej: ABC123)
// ============================================================
bool placa_es_valida(const std::string& placa) {
    if (placa.size() != 6) {
        return false;
    }
    // Las primeras 3 posiciones deben ser letras
    for (int indice = 0; indice < 3; indice++) {
        if (!((placa[indice] >= 'A' && placa[indice] <= 'Z') ||
              (placa[indice] >= 'a' && placa[indice] <= 'z'))) {
            return false;
        }
    }
    // Las ultimas 3 posiciones deben ser numeros
    for (int indice = 3; indice < 6; indice++) {
        if (!(placa[indice] >= '0' && placa[indice] <= '9')) {
            return false;
        }
    }
    return true;
}

// ============================================================
//  VERIFICAR SI UNA PLACA YA ESTA REGISTRADA Y ACTIVA
// ============================================================
bool placa_ya_registrada(const std::string& placa) {
    for (int indice = 0; indice < (int)placas.size(); indice++) {
        if (placas[indice] == placa && activos[indice] == true) {
            return true;
        }
    }
    return false;
}

// ============================================================
//  REGISTRAR INGRESO DE VEHICULO
// ============================================================
void registrar_ingreso() {
    std::cout << "\n  ---- INGRESO DE VEHICULO ----\n";

    // Verificar si hay espacio antes de pedir datos
    if (contar_espacios_libres() == 0) {
        std::cout << "  !! Parqueadero LLENO. No hay espacios disponibles.\n";
        return;
    }

    std::string placa;
    std::cout << "  Ingrese la placa (formato ABC123): ";
    std::cin >> placa;

    // Convertir cada letra a mayuscula manualmente
    for (int indice = 0; indice < (int)placa.size(); indice++) {
        if (placa[indice] >= 'a' && placa[indice] <= 'z') {
            placa[indice] = placa[indice] - 32;
        }
    }

    // Validar que la placa tenga formato correcto
    if (!placa_es_valida(placa)) {
        std::cout << "  !! Placa invalida. Use 3 letras + 3 numeros (ej: ABC123).\n";
        return;
    }

    // Verificar que ese vehiculo no este ya dentro
    if (placa_ya_registrada(placa)) {
        std::cout << "  !! La placa " << placa << " ya esta en el parqueadero.\n";
        return;
    }

    // Buscar el primer espacio libre en el mapa
    int fila_espacio    = 0;
    int columna_espacio = 0;
    if (!buscar_primer_espacio_libre(&fila_espacio, &columna_espacio)) {
        std::cout << "  !! No se encontro espacio disponible.\n";
        return;
    }

    // Avanzar el reloj 5 minutos por cada ingreso registrado
    minuto_actual += 5;
    if (minuto_actual >= 60) {
        minuto_actual -= 60;
        hora_actual++;
        if (hora_actual >= 24) hora_actual = 0;
    }

    // Agregar los datos del vehiculo en la misma posicion de cada vector
    placas          .push_back(placa);
    horas_entrada   .push_back(hora_actual);
    minutos_entrada .push_back(minuto_actual);
    filas_espacio   .push_back(fila_espacio);
    columnas_espacio.push_back(columna_espacio);
    activos         .push_back(true);

    // Marcar ese espacio como ocupado en el mapa
    mapa[fila_espacio][columna_espacio] = OCUPADO;

    // Mostrar la ruta desde la entrada hasta el espacio asignado
    mostrar_ruta_hasta_espacio(fila_espacio, columna_espacio);

    std::cout << "  ** Placa registrada  : " << placa << "\n";
    std::cout << "  ** Hora de ingreso   : " << hora_actual << ":"
              << (minuto_actual < 10 ? "0" : "") << minuto_actual << "\n";
    std::cout << "  ** Espacio asignado  : fila " << fila_espacio
              << ", columna " << columna_espacio << "\n";
    std::cout << "  ** Espacios restantes: " << contar_espacios_libres() << "\n\n";

    // Guardar el ingreso en el archivo de historial
    std::ostringstream registro;
    registro << "[INGRESO] Placa:" << placa
             << " Hora:" << hora_actual << ":" << minuto_actual
             << " Espacio:(" << fila_espacio << "," << columna_espacio << ")";
    guardar_en_historial(registro.str());
}

// ============================================================
//  CALCULAR EL VALOR A PAGAR SEGUN EL TIEMPO DE PERMANENCIA
// ============================================================
double calcular_valor(int hora_entrada, int minuto_entrada,
                      int hora_salida,  int minuto_salida) {
    int minutos_totales = (hora_salida  * 60 + minuto_salida)
                        - (hora_entrada * 60 + minuto_entrada);

    // Si el resultado es negativo significa que paso la medianoche
    if (minutos_totales < 0) {
        minutos_totales += 24 * 60;
    }

    // Cobro minimo de 1 hora aunque haya estado menos tiempo
    if (minutos_totales < 60) {
        minutos_totales = 60;
    }

    return (double)minutos_totales * TARIFA_POR_MINUTO;
}

// ============================================================
//  REGISTRAR SALIDA DE VEHICULO
//  El usuario ingresa la hora real de salida
// ============================================================
void registrar_salida() {
    std::cout << "\n  ---- SALIDA DE VEHICULO ----\n";

    if (placas.empty() || contar_espacios_ocupados() == 0) {
        std::cout << "  !! No hay vehiculos en el parqueadero.\n";
        return;
    }

    std::string placa;
    std::cout << "  Ingrese la placa del vehiculo a retirar: ";
    std::cin >> placa;

    // Convertir a mayusculas
    for (int indice = 0; indice < (int)placa.size(); indice++) {
        if (placa[indice] >= 'a' && placa[indice] <= 'z') {
            placa[indice] = placa[indice] - 32;
        }
    }

    // Buscar la placa dentro de los vectores
    bool encontrado = false;
    for (int indice = 0; indice < (int)placas.size(); indice++) {
        if (placas[indice] == placa && activos[indice] == true) {
            encontrado = true;

            // Pedir la hora de salida directamente al usuario
            int hora_salida   = 0;
            int minuto_salida = 0;
            std::cout << "  Ingrese la hora de salida (0-23): ";
            std::cin >> hora_salida;
            std::cout << "  Ingrese el minuto de salida (0-59): ";
            std::cin >> minuto_salida;

            // Validar que los valores sean correctos
            if (hora_salida < 0 || hora_salida > 23 ||
                minuto_salida < 0 || minuto_salida > 59) {
                std::cout << "  !! Hora invalida. Horas entre 0-23 y minutos entre 0-59.\n";
                return;
            }

            // Actualizar el reloj global a la hora de salida ingresada
            hora_actual   = hora_salida;
            minuto_actual = minuto_salida;

            // Calcular el cobro con los datos del vehiculo en la posicion indice
            double valor = calcular_valor(
                horas_entrada[indice],
                minutos_entrada[indice],
                hora_salida,
                minuto_salida
            );

            int tiempo_permanencia = (hora_salida        * 60 + minuto_salida)
                                   - (horas_entrada[indice] * 60 + minutos_entrada[indice]);
            if (tiempo_permanencia < 0) tiempo_permanencia += 24 * 60;

            // Liberar el espacio en el mapa
            mapa[filas_espacio[indice]][columnas_espacio[indice]] = LIBRE;

            // Marcar el vehiculo como inactivo
            activos[indice] = false;

            std::cout << "\n  ** COMPROBANTE DE COBRO **\n";
            std::cout << "  Placa         : " << placa << "\n";
            std::cout << "  Hora ingreso  : " << horas_entrada[indice]
                      << ":" << (minutos_entrada[indice] < 10 ? "0" : "")
                      << minutos_entrada[indice] << "\n";
            std::cout << "  Hora salida   : " << hora_salida << ":"
                      << (minuto_salida < 10 ? "0" : "") << minuto_salida << "\n";
            std::cout << "  Permanencia   : " << tiempo_permanencia << " minutos\n";
            std::cout << "  Tarifa/minuto : $" << TARIFA_POR_MINUTO << "\n";
            std::cout << "  TOTAL A PAGAR : $" << valor << " COP\n\n";

            // Guardar la salida en el archivo de historial
            std::ostringstream registro;
            registro << "[SALIDA] Placa:" << placa
                     << " Ingreso:" << horas_entrada[indice]
                     << ":" << minutos_entrada[indice]
                     << " Salida:" << hora_salida << ":" << minuto_salida
                     << " Tiempo:" << tiempo_permanencia << "min"
                     << " Cobro:$" << valor;
            guardar_en_historial(registro.str());

            break;
        }
    }

    if (!encontrado) {
        std::cout << "  !! No se encontro la placa " << placa << " en el parqueadero.\n";
    }
}

// ============================================================
//  MOSTRAR LISTA DE VEHICULOS QUE SIGUEN ADENTRO
// ============================================================
void mostrar_vehiculos_activos() {
    std::cout << "\n  ---- VEHICULOS ACTUALMENTE EN EL PARQUEADERO ----\n";

    int cantidad = 0;
    for (int indice = 0; indice < (int)placas.size(); indice++) {
        if (activos[indice]) {
            cantidad++;
            std::cout << "  [" << cantidad << "] "
                      << placas[indice]
                      << "  Ingreso: " << horas_entrada[indice]
                      << ":" << (minutos_entrada[indice] < 10 ? "0" : "")
                      << minutos_entrada[indice]
                      << "  Espacio: (" << filas_espacio[indice]
                      << "," << columnas_espacio[indice] << ")\n";
        }
    }

    if (cantidad == 0) {
        std::cout << "  (El parqueadero esta vacio)\n";
    } else {
        std::cout << "\n  Total vehiculos: " << cantidad << "\n";
    }
    std::cout << "\n";
}

// ============================================================
//  MOSTRAR ESTADISTICAS DE OCUPACION  (innovacion)
//  Barra visual + alerta si el parqueadero supera el 80%
// ============================================================
void mostrar_estadisticas() {
    int total_espacios = contar_espacios_libres() + contar_espacios_ocupados();
    int ocupados       = contar_espacios_ocupados();
    int libres         = contar_espacios_libres();
    double porcentaje  = (total_espacios > 0)
                        ? ((double)ocupados / total_espacios) * 100.0
                        : 0.0;

    std::cout << "\n  ---- ESTADISTICAS DE OCUPACION ----\n";
    std::cout << "  Hora actual    : " << hora_actual << ":"
              << (minuto_actual < 10 ? "0" : "") << minuto_actual << "\n";
    std::cout << "  Total espacios : " << total_espacios << "\n";
    std::cout << "  Ocupados       : " << ocupados << "\n";
    std::cout << "  Libres         : " << libres << "\n";
    std::cout << "  Ocupacion      : " << (int)porcentaje << "%\n";

    // Llamar a la funcion dibujar_barra (cumple el requisito de lambda del parcial)
    dibujar_barra(porcentaje, 30);

    // Alerta cuando el parqueadero supera el 80% de capacidad
    if (porcentaje >= 80.0) {
        std::cout << "\033[33m  !! ATENCION: Parqueadero al " << (int)porcentaje
                  << "% de capacidad.\033[0m\n\n";
    }
}

// ============================================================
//  BUSCAR VEHICULO POR PLACA  (innovacion: consulta rapida)
//  Muestra su estado y el cobro estimado si sigue adentro
// ============================================================
void buscar_vehiculo() {
    std::cout << "\n  ---- BUSCAR VEHICULO ----\n";
    std::string placa;
    std::cout << "  Ingrese la placa a buscar: ";
    std::cin >> placa;

    // Convertir a mayusculas
    for (int indice = 0; indice < (int)placa.size(); indice++) {
        if (placa[indice] >= 'a' && placa[indice] <= 'z') {
            placa[indice] = placa[indice] - 32;
        }
    }

    bool hallado = false;
    for (int indice = 0; indice < (int)placas.size(); indice++) {
        if (placas[indice] == placa) {
            hallado = true;

            std::string estado = activos[indice] ? "ADENTRO" : "YA SALIO";
            std::cout << "  Placa   : " << placas[indice] << "\n";
            std::cout << "  Estado  : " << estado << "\n";

            // Si sigue activo mostrar su cobro estimado hasta ahora
            if (activos[indice]) {
                std::cout << "  Ingreso : " << horas_entrada[indice]
                          << ":" << (minutos_entrada[indice] < 10 ? "0" : "")
                          << minutos_entrada[indice] << "\n";
                std::cout << "  Espacio : (" << filas_espacio[indice]
                          << "," << columnas_espacio[indice] << ")\n";

                double valor_actual = calcular_valor(
                    horas_entrada[indice],
                    minutos_entrada[indice],
                    hora_actual,
                    minuto_actual
                );
                std::cout << "  Cobro estimado hasta ahora: $" << valor_actual << " COP\n";
            }
        }
    }

    if (!hallado) {
        std::cout << "  No se encontraron registros para la placa " << placa << ".\n";
    }
    std::cout << "\n";
}

// ============================================================
//  VER HISTORIAL GUARDADO EN ARCHIVO  (innovacion: persistencia)
// ============================================================
void mostrar_historial() {
    std::cout << "\n  ---- HISTORIAL DE MOVIMIENTOS ----\n";
    std::ifstream archivo("historial_parqueadero.txt");
    if (!archivo.is_open()) {
        std::cout << "  (No hay historial guardado todavia)\n\n";
        return;
    }

    std::string linea;
    int contador = 0;
    while (std::getline(archivo, linea)) {
        std::cout << "  " << linea << "\n";
        contador++;
    }
    archivo.close();

    if (contador == 0) {
        std::cout << "  (Historial vacio)\n";
    } else {
        std::cout << "\n  Total de registros: " << contador << "\n";
    }
    std::cout << "\n";
}

// ============================================================
//  MENU PRINCIPAL
// ============================================================
void mostrar_menu() {
    std::cout << "\033[34m";
    std::cout << "\n  ========================================\n";
    std::cout << "       SISTEMA DE PARQUEADERO v1.0\n";
    std::cout << "  ========================================\n";
    std::cout << "\033[0m";
    std::cout << "  Hora: " << hora_actual << ":"
              << (minuto_actual < 10 ? "0" : "") << minuto_actual
              << "  |  Libres: "   << contar_espacios_libres()
              << "  |  Ocupados: " << contar_espacios_ocupados() << "\n\n";
    std::cout << "  [1] Ver mapa del parqueadero\n";
    std::cout << "  [2] Registrar ingreso de vehiculo\n";
    std::cout << "  [3] Registrar salida de vehiculo\n";
    std::cout << "  [4] Ver vehiculos activos\n";
    std::cout << "  [5] Estadisticas de ocupacion\n";
    std::cout << "  [6] Buscar vehiculo por placa\n";
    std::cout << "  [7] Ver historial de movimientos\n";
    std::cout << "  [0] Salir\n";
    std::cout << "  ----------------------------------------\n";
    std::cout << "  Opcion: ";
}

// ============================================================
//  FUNCION PRINCIPAL
//  main() solo coordina: construir el mapa y llamar funciones
// ============================================================
int main() {
    // Construir el mapa del parqueadero al arrancar el programa
    construir_mapa();

    // Dejar registro del inicio de sesion en el archivo
    guardar_en_historial("=== NUEVA SESION DEL SISTEMA ===");

    int opcion = -1;

    // Ciclo principal con do-while: muestra el menu al menos una vez
    do {
        mostrar_menu();
        std::cin >> opcion;

        // Si el usuario escribio letras en vez de un numero, limpiar el error
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            opcion = -1;
        }

        // Ejecutar la opcion elegida con switch
        switch (opcion) {
            case 1:
                mostrar_mapa();
                break;
            case 2:
                registrar_ingreso();
                break;
            case 3:
                registrar_salida();
                break;
            case 4:
                mostrar_vehiculos_activos();
                break;
            case 5:
                mostrar_estadisticas();
                break;
            case 6:
                buscar_vehiculo();
                break;
            case 7:
                mostrar_historial();
                break;
            case 0:
                std::cout << "\n  Cerrando sistema de parqueadero. Hasta pronto!\n\n";
                break;
            default:
                std::cout << "  !! Opcion invalida. Intente de nuevo.\n";
                break;
        }

    } while (opcion != 0);

    return 0;
}
