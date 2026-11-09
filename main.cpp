//  PROYECTO FINAL - ALGORITMOS
//  SISTEMA DE INVENTARIO Y VENTAS (Variante B: Farmacia)


//  1. LIBRERIAS Y CONSTANTES

#include <iostream>   
#include <fstream>    
#include <sstream>    
#include <iomanip>    
#include <string>    
#include <ctime>      
#include <cstdlib>    
using namespace std;

// Tamanos maximos de los arreglos (en C++ de este nivel los arreglos son fijos)
const int MAX_USUARIOS = 50;
const int MAX_PRODUCTOS = 100;
const int MAX_LOTES = 300;
const int MAX_CLIENTES = 100;
const int MAX_SOLICITUDES = 100;
const int MAX_PRESUPUESTOS = 100;
const int MAX_ENVIOS = 100;
const int MAX_VENTAS = 500;
const int MAX_ITEMS = 10;        // productos distintos por pedido
const int MAX_LINEAS = 40;       // lotes distintos que puede llevar un envio
const int NUM_REPARTIDORES = 3;  // la casa medica solo tiene 3 repartidores
const int STOCK_BAJO = 10;       // menos de esta cantidad = "stock bajo"
const int DIAS_ALERTA = 30;      // dias antes del vencimiento para dar alerta
const string CASA = "CASA MEDICA"; // nombre del "dueno" del inventario de la distribuidora


// ====================================================================
//  2. ESTRUCTURAS DE DATOS
//  Un struct agrupa datos relacionados bajo un solo nombre.
//  Corresponden al diccionario de datos de la Entrega 1.
// ====================================================================

// Persona que puede entrar al sistema
struct Usuario {
    string nombre;
    string clave;
    int tipo;            // 1 = casa medica, 2 = personal de farmacia, 3 = ENCARGADO de farmacia
    string nomFarmacia;  // solo se usa si tipo es 2 o 3
};

// Ficha general de un producto (no incluye cantidades, eso va en el Lote)
struct Producto {
    string cod;
    string nombre;
    float precio;
};

// Una tanda de un producto con su propio stock y fecha de vencimiento.
// Un mismo producto puede tener varios lotes.
struct Lote {
    string cod;      // codigo del producto al que pertenece
    int numLote;
    int stock;
    string fecha;    // vencimiento, formato AAAA-MM-DD
    string dueno;    // CASA o el nombre de la farmacia duena de ese stock
};

// Farmacia que le compra a la casa medica
struct Cliente {
    string nombreFarmacia;
    string telefono;
    string direccion;
};

// Una linea de un pedido: que producto y cuanto
struct Item {
    string cod;
    int cantidad;
};

// Pedido que arma la farmacia y manda a la casa medica
struct Solicitud {
    int id;
    string farmacia;
    string fecha;
    string estado;       // "Nueva", "Atendida" o "Cancelada"
    int numItems;
    Item items[MAX_ITEMS];
};

// Presupuesto que genera la casa medica a partir de una solicitud
struct Presupuesto {
    int id;
    string fecha;
    string cliente;
    int idSolicitud;
    float total;
    string estado;       // "Sin envio", "Con envio" o "Cancelado"
    int numItems;
    Item items[MAX_ITEMS];
};

// Que lote y cuanto salio para un envio (sirve para pasarlo a la farmacia al entregar)
struct LineaEnvio {
    string cod;
    int numLote;
    int cantidad;
    string fecha;
};

// Envio de un presupuesto a una farmacia
struct Envio {
    int id;
    int idPresupuesto;
    string cliente;
    int repartidor;      // 0 = todavia sin repartidor
    string estado;       // "Pendiente", "En camino", "Entregado" o "Cancelado"
    int tiempo;          // minutos estimados de entrega
    int numLineas;
    LineaEnvio lineas[MAX_LINEAS];
};

// Una linea de una venta al publico. Una venta (presupuesto de venta) puede tener
// varias lineas: todas comparten el mismo idVenta. Se guarda quien la hizo.
struct Venta {
    int idVenta;       // numero de la venta (igual para todas sus lineas)
    string farmacia;
    string fecha;
    string vendedor;   // usuario que hizo la venta
    string cod;
    int numLote;
    int cantidad;
    float total;
    string estado;     // "Activa" o "Cancelada"
};

// Agrupa TODOS los datos del sistema en un solo lugar.
// Asi pasamos un solo parametro a las funciones en vez de muchos arreglos.
struct Sistema {
    Usuario usuarios[MAX_USUARIOS];             int totalUsuarios;
    Producto productos[MAX_PRODUCTOS];          int totalProductos;
    Lote lotes[MAX_LOTES];                      int totalLotes;
    Cliente clientes[MAX_CLIENTES];             int totalClientes;
    Solicitud solicitudes[MAX_SOLICITUDES];     int totalSolicitudes;
    Presupuesto presupuestos[MAX_PRESUPUESTOS]; int totalPresupuestos;
    Envio envios[MAX_ENVIOS];                   int totalEnvios;
    Venta ventas[MAX_VENTAS];                   int totalVentas;
};


// ====================================================================
//  3. UTILIDADES: TEXTO, LECTURA VALIDADA Y FECHAS
//  Funciones pequenas que usa todo el programa.
// ====================================================================

// Convierte un numero entero a texto (ej. 25 -> "25")
string aTexto(int v) {
    ostringstream o;
    o << v;
    return o.str();
}

// Quita los espacios del inicio y del final de un texto
string recortar(const string &t) {
    size_t i = 0, j = t.size();
    while (i < j && (t[i] == ' ' || t[i] == '\t' || t[i] == '\r')) i++;
    while (j > i && (t[j - 1] == ' ' || t[j - 1] == '\t' || t[j - 1] == '\r')) j--;
    return t.substr(i, j - i);
}

// Pasa un texto a MAYUSCULAS (para que los codigos P001 y p001 sean iguales)
string mayusculas(string t) {
    for (size_t i = 0; i < t.size(); i++)
        if (t[i] >= 'a' && t[i] <= 'z') t[i] = t[i] - 'a' + 'A';
    return t;
}

// Pasa un texto a minusculas (sirve para comparar sin importar mayusculas)
string minusculas(string t) {
    for (size_t i = 0; i < t.size(); i++)
        if (t[i] >= 'A' && t[i] <= 'Z') t[i] = t[i] - 'A' + 'a';
    return t;
}

// Dice si 'texto' contiene a 'parte' (sin importar mayusculas). Sirve para buscar por nombre.
bool contiene(const string &texto, const string &parte) {
    return minusculas(texto).find(minusculas(parte)) != string::npos;
}

// Acorta un texto largo para que quepa en una columna de la tabla
string cortar(const string &t, size_t limite) {
    if (t.size() <= limite) return t;
    return t.substr(0, limite - 1) + ".";
}

// Convierte un texto con solo digitos a entero. Si trae algo raro devuelve 0.
int aEntero(const string &t) {
    int v = 0;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] < '0' || t[i] > '9') return 0;
        v = v * 10 + (t[i] - '0');
    }
    return v;
}

// Convierte un texto a numero decimal
float aDecimal(const string &t) {
    istringstream iss(t);
    float v = 0;
    iss >> v;
    return v;
}

// Da formato de dinero: 15.5 -> "Q15.50"
string dinero(float v) {
    ostringstream o;
    o << "Q" << fixed << setprecision(2) << v;
    return o.str();
}

// Imprime un encabezado con lineas para separar las pantallas
void titulo(const string &t) {
    cout << "\n==============================================\n";
    cout << "  " << t << "\n";
    cout << "==============================================\n";
}

// Lee una linea completa del teclado. Si se acaba la entrada, cierra el programa.
string leerLinea(const string &msg) {
    string t;
    cout << msg;
    if (!getline(cin, t)) {
        cout << "\n[Entrada finalizada]\n";
        exit(0);
    }
    return t;
}

// Espera a que el usuario presione ENTER (para que le de tiempo de leer)
void pausar() {
    string t;
    cout << "\nPresione ENTER para continuar...";
    getline(cin, t);
}

// Lee un texto que NO puede estar vacio.
// Cambia el simbolo '|' por '-' porque '|' lo usamos para separar campos en los archivos.
string leerTexto(const string &msg) {
    while (true) {
        string t = recortar(leerLinea(msg));
        if (t.empty()) {
            cout << "  El campo no puede estar vacio.\n";
            continue;
        }
        for (size_t i = 0; i < t.size(); i++)
            if (t[i] == '|') t[i] = '-';
        return t;
    }
}

// Lee un codigo y lo pasa a mayusculas
string leerCodigo(const string &msg) {
    return mayusculas(leerTexto(msg));
}

// Lee un numero entero entre 'minimo' y 'maximo'.
// Si el usuario escribe letras o un numero fuera de rango, vuelve a preguntar.
// Esto evita que el programa se caiga con entradas incorrectas.
int leerEntero(const string &msg, int minimo, int maximo) {
    while (true) {
        string t = recortar(leerLinea(msg));
        bool ok = !t.empty() && t.size() <= 9;
        for (size_t i = 0; i < t.size() && ok; i++)
            if (t[i] < '0' || t[i] > '9') ok = false;
        if (ok) {
            int v = aEntero(t);
            if (v >= minimo && v <= maximo) return v;
        }
        cout << "  Valor invalido. Ingrese un numero entre " << minimo << " y " << maximo << ".\n";
    }
}

// Lee un precio (decimal mayor que 0)
float leerPrecio(const string &msg) {
    while (true) {
        string t = recortar(leerLinea(msg));
        istringstream iss(t);
        float v;
        char extra;
        if ((iss >> v) && !(iss >> extra) && v > 0 && v < 1000000) return v;
        cout << "  Precio invalido. Debe ser un numero mayor que 0.\n";
    }
}

// Lee un telefono: exactamente 8 digitos
string leerTelefono(const string &msg) {
    while (true) {
        string t = recortar(leerLinea(msg));
        bool ok = (t.size() == 8);
        for (size_t i = 0; i < t.size() && ok; i++)
            if (t[i] < '0' || t[i] > '9') ok = false;
        if (ok) return t;
        cout << "  Telefono invalido. Deben ser 8 digitos.\n";
    }
}

// Pregunta s/n y devuelve true si la respuesta fue si
bool confirmar(const string &msg) {
    while (true) {
        string t = minusculas(recortar(leerLinea(msg + " (s/n): ")));
        if (t == "s" || t == "si") return true;
        if (t == "n" || t == "no") return false;
        cout << "  Responda s o n.\n";
    }
}

// ---------------------------- FECHAS ----------------------------
// Las fechas se guardan como texto "AAAA-MM-DD". Con ese formato,
// comparar textos (<, >) equivale a comparar fechas.

// Devuelve la fecha de hoy tomada del reloj de la computadora
string fechaHoy() {
    time_t t = time(0);
    tm *lt = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return string(buf);
}

// Dice si un ano es bisiesto (febrero tiene 29 dias)
bool esBisiesto(int a) {
    return (a % 4 == 0 && a % 100 != 0) || (a % 400 == 0);
}

// Cuantos dias tiene el mes m del ano a
int diasDelMes(int m, int a) {
    int dias[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && esBisiesto(a)) return 29;
    return dias[m - 1];
}

// Revisa que el texto sea una fecha real con formato AAAA-MM-DD
bool fechaValida(const string &f) {
    if (f.size() != 10 || f[4] != '-' || f[7] != '-') return false;
    int pos[8] = {0, 1, 2, 3, 5, 6, 8, 9};
    for (int i = 0; i < 8; i++)
        if (f[pos[i]] < '0' || f[pos[i]] > '9') return false;
    int a = aEntero(f.substr(0, 4));
    int m = aEntero(f.substr(5, 2));
    int d = aEntero(f.substr(8, 2));
    if (a < 2000 || a > 2100 || m < 1 || m > 12) return false;
    return d >= 1 && d <= diasDelMes(m, a);
}

// Lee una fecha y no la acepta hasta que sea valida
string leerFecha(const string &msg) {
    while (true) {
        string t = recortar(leerLinea(msg));
        if (fechaValida(t)) return t;
        cout << "  Fecha invalida. Use el formato AAAA-MM-DD (ej. 2027-03-15).\n";
    }
}

// Lee la fecha de vencimiento de un lote: debe ser una fecha valida y NO estar vencida
string leerFechaVencimiento(const string &msg) {
    while (true) {
        string f = leerFecha(msg);
        if (f >= fechaHoy()) return f;
        cout << "  Esa fecha ya paso: no se puede registrar un lote vencido.\n";
    }
}

// Cuantos dias hay entre dos fechas (hasta - desde). Sirve para las alertas de vencimiento.
int diasEntre(const string &desde, const string &hasta) {
    tm a = tm(), b = tm();
    a.tm_year = aEntero(desde.substr(0, 4)) - 1900;
    a.tm_mon = aEntero(desde.substr(5, 2)) - 1;
    a.tm_mday = aEntero(desde.substr(8, 2));
    a.tm_hour = 12;
    b.tm_year = aEntero(hasta.substr(0, 4)) - 1900;
    b.tm_mon = aEntero(hasta.substr(5, 2)) - 1;
    b.tm_mday = aEntero(hasta.substr(8, 2));
    b.tm_hour = 12;
    double seg = difftime(mktime(&b), mktime(&a));
    return (int)(seg / 86400.0 + (seg >= 0 ? 0.5 : -0.5));
}

// ====================================================================
//  4. BUSQUEDAS (busqueda secuencial hecha a mano)
//  Recorren el arreglo de principio a fin. Devuelven la POSICION donde
//  encontraron lo buscado, o -1 si no existe.
// ====================================================================

int buscarUsuario(const Sistema &s, const string &nombre) {
    for (int i = 0; i < s.totalUsuarios; i++)
        if (s.usuarios[i].nombre == nombre) return i;
    return -1;
}

int buscarProducto(const Sistema &s, const string &cod) {
    for (int i = 0; i < s.totalProductos; i++)
        if (s.productos[i].cod == cod) return i;
    return -1;
}

int buscarCliente(const Sistema &s, const string &nombre) {
    for (int i = 0; i < s.totalClientes; i++)
        if (minusculas(s.clientes[i].nombreFarmacia) == minusculas(nombre)) return i;
    return -1;
}

// Un lote se identifica por: codigo de producto + numero de lote + dueno
int buscarLote(const Sistema &s, const string &cod, int num, const string &dueno) {
    for (int i = 0; i < s.totalLotes; i++)
        if (s.lotes[i].cod == cod && s.lotes[i].numLote == num && s.lotes[i].dueno == dueno) return i;
    return -1;
}

// Busca un producto dentro de la lista de un pedido
int buscarItem(const Item items[], int n, const string &cod) {
    for (int i = 0; i < n; i++)
        if (items[i].cod == cod) return i;
    return -1;
}

// Devuelve la posicion del ENCARGADO (tipo 3) de una farmacia, o -1 si todavia no tiene
int buscarEncargado(const Sistema &s, const string &farmacia) {
    for (int i = 0; i < s.totalUsuarios; i++)
        if (s.usuarios[i].tipo == 3 && s.usuarios[i].nomFarmacia == farmacia) return i;
    return -1;
}

// Nombre del rol de un usuario, para mostrarlo en pantalla
string nombreTipo(int tipo) {
    if (tipo == 1) return "Casa medica";
    if (tipo == 3) return "Encargado";
    return "Personal";
}

// Devuelve el nombre de un producto a partir de su codigo
string nombreProducto(const Sistema &s, const string &cod) {
    int p = buscarProducto(s, cod);
    return (p == -1) ? "(desconocido)" : s.productos[p].nombre;
}


// ====================================================================
//  5. MANEJO DE STOCK
//  Aqui esta la logica central del inventario: que se puede vender,
//  como se descuenta y como se agrega.
// ====================================================================

// Suma el stock de un producto que se puede vender: solo lotes NO vencidos
// y que pertenezcan al 'dueno' indicado (casa medica o una farmacia).
int stockVigente(const Sistema &s, const string &cod, const string &dueno, const string &hoy) {
    int total = 0;
    for (int i = 0; i < s.totalLotes; i++)
        if (s.lotes[i].cod == cod && s.lotes[i].dueno == dueno &&
            s.lotes[i].stock > 0 && s.lotes[i].fecha >= hoy)
            total += s.lotes[i].stock;
    return total;
}

// Cuenta cuantos lotes vigentes tiene un producto
int lotesVigentes(const Sistema &s, const string &cod, const string &dueno, const string &hoy) {
    int n = 0;
    for (int i = 0; i < s.totalLotes; i++)
        if (s.lotes[i].cod == cod && s.lotes[i].dueno == dueno &&
            s.lotes[i].stock > 0 && s.lotes[i].fecha >= hoy)
            n++;
    return n;
}

// Saca 'cantidad' unidades de un producto, empezando por el lote que VENCE PRIMERO.
// Asi no se queda producto viejo sin vender. Va anotando en 'lineas' de que lotes salio.
// Devuelve false si no alcanza el stock vigente.
bool descontarStock(Sistema &s, const string &cod, const string &dueno, int cantidad,
                    LineaEnvio lineas[], int &nLineas) {
    string hoy = fechaHoy();
    if (stockVigente(s, cod, dueno, hoy) < cantidad) return false;
    if (nLineas + lotesVigentes(s, cod, dueno, hoy) > MAX_LINEAS) return false;
    int pendiente = cantidad;
    while (pendiente > 0) {
        // buscar el lote vigente con la fecha de vencimiento mas cercana
        int mejor = -1;
        for (int i = 0; i < s.totalLotes; i++) {
            if (s.lotes[i].cod == cod && s.lotes[i].dueno == dueno &&
                s.lotes[i].stock > 0 && s.lotes[i].fecha >= hoy) {
                if (mejor == -1 || s.lotes[i].fecha < s.lotes[mejor].fecha) mejor = i;
            }
        }
        // tomar del lote lo que se pueda (o lo que falte)
        int tomar = (pendiente < s.lotes[mejor].stock) ? pendiente : s.lotes[mejor].stock;
        lineas[nLineas].cod = cod;
        lineas[nLineas].numLote = s.lotes[mejor].numLote;
        lineas[nLineas].cantidad = tomar;
        lineas[nLineas].fecha = s.lotes[mejor].fecha;
        nLineas++;
        s.lotes[mejor].stock -= tomar;
        pendiente -= tomar;
    }
    return true;
}

// Suma stock a un lote del dueno indicado. Si el lote no existe, lo crea.
// Se usa al registrar un lote y al entregar un envio a una farmacia.
bool agregarStock(Sistema &s, const string &cod, int num, const string &fecha, int cantidad,
                  const string &dueno) {
    int l = buscarLote(s, cod, num, dueno);
    if (l != -1) {
        s.lotes[l].stock += cantidad;
        return true;
    }
    if (s.totalLotes >= MAX_LOTES) return false;
    Lote &n = s.lotes[s.totalLotes];
    s.totalLotes++;
    n.cod = cod;
    n.numLote = num;
    n.stock = cantidad;
    n.fecha = fecha;
    n.dueno = dueno;
    return true;
}


// ====================================================================
//  6. ORDENAMIENTO MANUAL (metodo de burbuja)
//  Ordena una lista de posiciones (idx) segun la fecha de vencimiento,
//  de la que vence primero a la que vence al final.
//  Ordenamos las POSICIONES, no los lotes, para no mover datos grandes.
// ====================================================================
void ordenarIndicesPorFecha(const Lote lotes[], int idx[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (lotes[idx[j]].fecha > lotes[idx[j + 1]].fecha) {
                int tmp = idx[j];
                idx[j] = idx[j + 1];
                idx[j + 1] = tmp;
            }
        }
    }
}


// ====================================================================
//  7. PERSISTENCIA (guardar y cargar con archivos de texto)
//  Cada dato se guarda en un .txt, una linea por registro, con los
//  campos separados por '|'. Al abrir el programa se cargan todos.
// ====================================================================

// Parte una linea en campos usando '|' como separador
int dividir(const string &linea, string partes[], int maxPartes) {
    int n = 0;
    string actual;
    for (size_t i = 0; i < linea.size(); i++) {
        if (linea[i] == '|') {
            if (n < maxPartes) { partes[n] = actual; n++; }
            actual.clear();
        } else {
            actual += linea[i];
        }
    }
    if (n < maxPartes) { partes[n] = actual; n++; }
    return n;
}

// Escribe TODOS los datos en sus archivos. Se llama despues de cada cambio
// para que no se pierda nada si el programa se cierra.
void guardarTodo(const Sistema &s) {
    {   // usuarios.txt
        ofstream f("usuarios.txt");
        for (int i = 0; i < s.totalUsuarios; i++)
            f << s.usuarios[i].nombre << '|' << s.usuarios[i].clave << '|'
              << s.usuarios[i].tipo << '|' << s.usuarios[i].nomFarmacia << '\n';
    }
    {   // productos.txt
        ofstream f("productos.txt");
        f << fixed << setprecision(2);
        for (int i = 0; i < s.totalProductos; i++)
            f << s.productos[i].cod << '|' << s.productos[i].nombre << '|' << s.productos[i].precio << '\n';
    }
    {   // lotes.txt
        ofstream f("lotes.txt");
        for (int i = 0; i < s.totalLotes; i++)
            f << s.lotes[i].cod << '|' << s.lotes[i].numLote << '|' << s.lotes[i].stock << '|'
              << s.lotes[i].fecha << '|' << s.lotes[i].dueno << '\n';
    }
    {   // clientes.txt
        ofstream f("clientes.txt");
        for (int i = 0; i < s.totalClientes; i++)
            f << s.clientes[i].nombreFarmacia << '|' << s.clientes[i].telefono << '|'
              << s.clientes[i].direccion << '\n';
    }
    {   // solicitudes.txt (al final de cada linea van los productos del pedido)
        ofstream f("solicitudes.txt");
        for (int i = 0; i < s.totalSolicitudes; i++) {
            const Solicitud &x = s.solicitudes[i];
            f << x.id << '|' << x.farmacia << '|' << x.fecha << '|' << x.estado << '|' << x.numItems;
            for (int k = 0; k < x.numItems; k++) f << '|' << x.items[k].cod << '|' << x.items[k].cantidad;
            f << '\n';
        }
    }
    {   // presupuestos.txt
        ofstream f("presupuestos.txt");
        f << fixed << setprecision(2);
        for (int i = 0; i < s.totalPresupuestos; i++) {
            const Presupuesto &x = s.presupuestos[i];
            f << x.id << '|' << x.fecha << '|' << x.cliente << '|' << x.idSolicitud << '|'
              << x.total << '|' << x.estado << '|' << x.numItems;
            for (int k = 0; k < x.numItems; k++) f << '|' << x.items[k].cod << '|' << x.items[k].cantidad;
            f << '\n';
        }
    }
    {   // envios.txt
        ofstream f("envios.txt");
        for (int i = 0; i < s.totalEnvios; i++) {
            const Envio &x = s.envios[i];
            f << x.id << '|' << x.idPresupuesto << '|' << x.cliente << '|' << x.repartidor << '|'
              << x.estado << '|' << x.tiempo << '|' << x.numLineas;
            for (int k = 0; k < x.numLineas; k++)
                f << '|' << x.lineas[k].cod << '|' << x.lineas[k].numLote << '|'
                  << x.lineas[k].cantidad << '|' << x.lineas[k].fecha;
            f << '\n';
        }
    }
    {   // ventas.txt
        ofstream f("ventas.txt");
        f << fixed << setprecision(2);
        for (int i = 0; i < s.totalVentas; i++) {
            const Venta &x = s.ventas[i];
            f << x.idVenta << '|' << x.farmacia << '|' << x.fecha << '|' << x.vendedor << '|'
              << x.cod << '|' << x.numLote << '|' << x.cantidad << '|' << x.total << '|' << x.estado << '\n';
        }
    }
}

// Las funciones cargarXxx leen un archivo y llenan el arreglo correspondiente.
// Si el archivo no existe (primera vez), simplemente queda vacio.

void cargarUsuarios(Sistema &s) {
    s.totalUsuarios = 0;
    ifstream f("usuarios.txt");
    if (!f.is_open()) {   // primera ejecucion: se crea el usuario administrador
        s.usuarios[0].nombre = "admin";
        s.usuarios[0].clave = "1234";
        s.usuarios[0].tipo = 1;
        s.usuarios[0].nomFarmacia = "";
        s.totalUsuarios = 1;
        return;
    }
    string linea, p[8];
    while (getline(f, linea) && s.totalUsuarios < MAX_USUARIOS) {
        if (recortar(linea).empty()) continue;
        if (dividir(linea, p, 8) < 4) continue;
        Usuario &u = s.usuarios[s.totalUsuarios];
        s.totalUsuarios++;
        u.nombre = p[0];
        u.clave = p[1];
        u.tipo = aEntero(p[2]);
        u.nomFarmacia = p[3];
    }
}

void cargarProductos(Sistema &s) {
    s.totalProductos = 0;
    ifstream f("productos.txt");
    string linea, p[8];
    while (getline(f, linea) && s.totalProductos < MAX_PRODUCTOS) {
        if (recortar(linea).empty()) continue;
        if (dividir(linea, p, 8) < 3) continue;
        Producto &x = s.productos[s.totalProductos];
        s.totalProductos++;
        x.cod = p[0];
        x.nombre = p[1];
        x.precio = aDecimal(p[2]);
    }
}

void cargarLotes(Sistema &s) {
    s.totalLotes = 0;
    ifstream f("lotes.txt");
    string linea, p[8];
    while (getline(f, linea) && s.totalLotes < MAX_LOTES) {
        if (recortar(linea).empty()) continue;
        if (dividir(linea, p, 8) < 5) continue;
        Lote &x = s.lotes[s.totalLotes];
        s.totalLotes++;
        x.cod = p[0];
        x.numLote = aEntero(p[1]);
        x.stock = aEntero(p[2]);
        x.fecha = p[3];
        x.dueno = p[4];
    }
}

void cargarClientes(Sistema &s) {
    s.totalClientes = 0;
    ifstream f("clientes.txt");
    string linea, p[8];
    while (getline(f, linea) && s.totalClientes < MAX_CLIENTES) {
        if (recortar(linea).empty()) continue;
        if (dividir(linea, p, 8) < 3) continue;
        Cliente &x = s.clientes[s.totalClientes];
        s.totalClientes++;
        x.nombreFarmacia = p[0];
        x.telefono = p[1];
        x.direccion = p[2];
    }
}

void cargarSolicitudes(Sistema &s) {
    s.totalSolicitudes = 0;
    ifstream f("solicitudes.txt");
    string linea, p[40];
    while (getline(f, linea) && s.totalSolicitudes < MAX_SOLICITUDES) {
        if (recortar(linea).empty()) continue;
        int n = dividir(linea, p, 40);
        if (n < 5) continue;
        int k = aEntero(p[4]);
        if (k > MAX_ITEMS || n < 5 + 2 * k) continue;
        Solicitud &x = s.solicitudes[s.totalSolicitudes];
        s.totalSolicitudes++;
        x.id = aEntero(p[0]);
        x.farmacia = p[1];
        x.fecha = p[2];
        x.estado = p[3];
        x.numItems = k;
        for (int i = 0; i < k; i++) {
            x.items[i].cod = p[5 + 2 * i];
            x.items[i].cantidad = aEntero(p[6 + 2 * i]);
        }
    }
}

void cargarPresupuestos(Sistema &s) {
    s.totalPresupuestos = 0;
    ifstream f("presupuestos.txt");
    string linea, p[40];
    while (getline(f, linea) && s.totalPresupuestos < MAX_PRESUPUESTOS) {
        if (recortar(linea).empty()) continue;
        int n = dividir(linea, p, 40);
        if (n < 7) continue;
        int k = aEntero(p[6]);
        if (k > MAX_ITEMS || n < 7 + 2 * k) continue;
        Presupuesto &x = s.presupuestos[s.totalPresupuestos];
        s.totalPresupuestos++;
        x.id = aEntero(p[0]);
        x.fecha = p[1];
        x.cliente = p[2];
        x.idSolicitud = aEntero(p[3]);
        x.total = aDecimal(p[4]);
        x.estado = p[5];
        x.numItems = k;
        for (int i = 0; i < k; i++) {
            x.items[i].cod = p[7 + 2 * i];
            x.items[i].cantidad = aEntero(p[8 + 2 * i]);
        }
    }
}

void cargarEnvios(Sistema &s) {
    s.totalEnvios = 0;
    ifstream f("envios.txt");
    string linea, p[160];
    while (getline(f, linea) && s.totalEnvios < MAX_ENVIOS) {
        if (recortar(linea).empty()) continue;
        int n = dividir(linea, p, 160);
        if (n < 7) continue;
        int k = aEntero(p[6]);
        if (k > MAX_LINEAS || n < 7 + 4 * k) continue;
        Envio &x = s.envios[s.totalEnvios];
        s.totalEnvios++;
        x.id = aEntero(p[0]);
        x.idPresupuesto = aEntero(p[1]);
        x.cliente = p[2];
        x.repartidor = aEntero(p[3]);
        x.estado = p[4];
        x.tiempo = aEntero(p[5]);
        x.numLineas = k;
        for (int i = 0; i < k; i++) {
            x.lineas[i].cod = p[7 + 4 * i];
            x.lineas[i].numLote = aEntero(p[8 + 4 * i]);
            x.lineas[i].cantidad = aEntero(p[9 + 4 * i]);
            x.lineas[i].fecha = p[10 + 4 * i];
        }
    }
}

void cargarVentas(Sistema &s) {
    s.totalVentas = 0;
    ifstream f("ventas.txt");
    string linea, p[10];
    while (getline(f, linea) && s.totalVentas < MAX_VENTAS) {
        if (recortar(linea).empty()) continue;
        int n = dividir(linea, p, 10);
        if (n < 8) continue;
        Venta &x = s.ventas[s.totalVentas];
        s.totalVentas++;
        x.idVenta = aEntero(p[0]);
        x.farmacia = p[1];
        x.fecha = p[2];
        x.vendedor = p[3];
        x.cod = p[4];
        x.numLote = aEntero(p[5]);
        x.cantidad = aEntero(p[6]);
        x.total = aDecimal(p[7]);
        x.estado = (n >= 9 && p[8] == "Cancelada") ? "Cancelada" : "Activa";
    }
}

// Carga todos los archivos de una vez al iniciar el programa
void cargarTodo(Sistema &s) {
    cargarUsuarios(s);
    cargarProductos(s);
    cargarLotes(s);
    cargarClientes(s);
    cargarSolicitudes(s);
    cargarPresupuestos(s);
    cargarEnvios(s);
    cargarVentas(s);
}

// ====================================================================
//  7B. DATOS INICIALES
//  Productos y lotes con los que arranca la Casa Medica la primera vez
//  (cuando todavia no hay archivos guardados).
//  - Lote A = lote 1, Lote B = lote 2.
//  - Los vencimientos vienen como mes/ano: se guarda el ULTIMO DIA de ese mes.
//  - Los precios (en quetzales) son de EJEMPLO: cambielos por los reales.
// ====================================================================

// Agrega un producto al catalogo (solo se usa para los datos iniciales)
bool agregarProductoInicial(Sistema &s, const string &cod, const string &nombre, float precio) {
    if (buscarProducto(s, cod) != -1) return false;   // ya existe: no se repite
    Producto &p = s.productos[s.totalProductos];
    s.totalProductos++;
    p.cod = cod;
    p.nombre = nombre;
    p.precio = precio;
    return true;
}

// Carga el catalogo y los lotes iniciales en el inventario de la Casa Medica
void cargarDatosIniciales(Sistema &s) {
    // --- Medicamentos de venta libre (OTC) ---
    if (agregarProductoInicial(s, "PA500", "Paracetamol 500mg x20", 18.00)) {
        agregarStock(s, "PA500", 1, "2028-05-31", 150, CASA);
        agregarStock(s, "PA500", 2, "2028-09-30", 200, CASA);
    }

    if (agregarProductoInicial(s, "IB400", "Ibuprofeno 400mg x10", 25.00)) {
        agregarStock(s, "IB400", 1, "2028-03-31", 90, CASA);
        agregarStock(s, "IB400", 2, "2028-07-31", 120, CASA);
    }

    if (agregarProductoInicial(s, "OM020", "Omeprazol 20mg x14", 35.00)) {
        agregarStock(s, "OM020", 1, "2027-11-30", 80, CASA);
    }

    // --- Antigripales y respiratorios ---
    if (agregarProductoInicial(s, "JT120", "Jarabe para tos 120ml", 32.00)) {
        agregarStock(s, "JT120", 1, "2027-04-30", 60, CASA);
        agregarStock(s, "JT120", 2, "2027-10-31", 75, CASA);
    }

    if (agregarProductoInicial(s, "DN015", "Desc. Nasal Spray 15ml", 38.00)) {
        agregarStock(s, "DN015", 1, "2028-01-31", 110, CASA);
    }

    // --- Parafarmacia y cuidado personal ---
    if (agregarProductoInicial(s, "PS050", "Protector Solar FPS 50+", 95.00)) {
        agregarStock(s, "PS050", 1, "2028-06-30", 45, CASA);
        agregarStock(s, "PS050", 2, "2028-12-31", 60, CASA);
    }

    if (agregarProductoInicial(s, "SO060", "Suero Oral 60g", 8.00)) {
        agregarStock(s, "SO060", 1, "2028-02-29", 300, CASA);
        agregarStock(s, "SO060", 2, "2028-08-31", 450, CASA);
    }
}


// ====================================================================
//  8. PANTALLAS DE CONSULTA COMPARTIDAS
//  Las usan tanto la Casa Medica como las Farmacias; solo cambia
//  el 'dueno' cuyo inventario se muestra.
// ====================================================================

// Muestra el inventario con las columnas: nombre, stock, vencimiento, lote.
// Los lotes salen ordenados por fecha de vencimiento (burbuja).
void mostrarInventario(const Sistema &s, const string &dueno) {
    int idx[MAX_LOTES];
    int n = 0;
    for (int i = 0; i < s.totalLotes; i++)
        if (s.lotes[i].dueno == dueno && s.lotes[i].stock > 0) { idx[n] = i; n++; }
    titulo("INVENTARIO - " + dueno);
    if (n == 0) {
        cout << "  No hay productos en existencia.\n";
        return;
    }
    ordenarIndicesPorFecha(s.lotes, idx, n);
    string hoy = fechaHoy();
    cout << left << setw(26) << "  Nombre" << setw(8) << "Stock" << setw(13) << "Vencimiento"
         << setw(7) << "Lote" << setw(9) << "Codigo" << "Estado\n";
    cout << "  " << string(70, '-') << "\n";
    for (int k = 0; k < n; k++) {
        const Lote &l = s.lotes[idx[k]];
        string estado = "";
        if (l.fecha < hoy) estado = "VENCIDO";
        else if (diasEntre(hoy, l.fecha) <= DIAS_ALERTA) estado = "Por vencer";
        cout << left << "  " << setw(24) << cortar(nombreProducto(s, l.cod), 24)
             << setw(8) << l.stock << setw(13) << l.fecha << setw(7) << l.numLote
             << setw(9) << l.cod << estado << "\n";
    }
}

// Lista los productos que tienen stock vigente (se usa al crear pedidos y ventas)
void mostrarCatalogoConStock(const Sistema &s, const string &dueno) {
    string hoy = fechaHoy();
    cout << left << "  " << setw(10) << "Codigo" << setw(26) << "Nombre" << setw(12) << "Precio"
         << "Disponible\n";
    cout << "  " << string(58, '-') << "\n";
    int mostrados = 0;
    for (int i = 0; i < s.totalProductos; i++) {
        int st = stockVigente(s, s.productos[i].cod, dueno, hoy);
        if (st > 0) {
            cout << left << "  " << setw(10) << s.productos[i].cod << setw(26)
                 << cortar(s.productos[i].nombre, 25) << setw(12) << dinero(s.productos[i].precio)
                 << st << "\n";
            mostrados++;
        }
    }
    if (mostrados == 0) cout << "  (no hay productos disponibles)\n";
}

// Muestra la lista de productos de un pedido o presupuesto.
// Si conPrecio es true, agrega precio y subtotal.
void mostrarItems(const Sistema &s, const Item items[], int n, bool conPrecio) {
    cout << left << "  " << setw(10) << "Codigo" << setw(26) << "Producto" << setw(10) << "Cantidad";
    if (conPrecio) cout << setw(12) << "Precio" << "Subtotal";
    cout << "\n  " << string(conPrecio ? 70 : 46, '-') << "\n";
    for (int i = 0; i < n; i++) {
        int p = buscarProducto(s, items[i].cod);
        cout << left << "  " << setw(10) << items[i].cod << setw(26)
             << cortar(nombreProducto(s, items[i].cod), 25) << setw(10) << items[i].cantidad;
        if (conPrecio && p != -1)
            cout << setw(12) << dinero(s.productos[p].precio)
                 << dinero(s.productos[p].precio * items[i].cantidad);
        cout << "\n";
    }
}

// Busqueda de productos por DOS criterios: codigo exacto o parte del nombre
void buscarProductos(const Sistema &s, const string &dueno) {
    titulo("BUSCAR PRODUCTO");
    cout << "  1. Por codigo\n  2. Por nombre (o parte del nombre)\n";
    int op = leerEntero("  Opcion: ", 1, 2);
    string hoy = fechaHoy();
    int encontrados = 0;
    if (op == 1) {
        string cod = leerCodigo("  Codigo: ");
        int p = buscarProducto(s, cod);
        if (p != -1) {
            cout << "  " << s.productos[p].cod << " - " << s.productos[p].nombre << " - "
                 << dinero(s.productos[p].precio) << " - stock vigente: "
                 << stockVigente(s, cod, dueno, hoy) << "\n";
            encontrados = 1;
        }
    } else {
        string parte = leerTexto("  Nombre contiene: ");
        for (int i = 0; i < s.totalProductos; i++) {
            if (contiene(s.productos[i].nombre, parte)) {
                cout << "  " << s.productos[i].cod << " - " << s.productos[i].nombre << " - "
                     << dinero(s.productos[i].precio) << " - stock vigente: "
                     << stockVigente(s, s.productos[i].cod, dueno, hoy) << "\n";
                encontrados++;
            }
        }
    }
    if (encontrados == 0) cout << "  No se encontraron productos.\n";
}

// Muestra la tabla de clientes
void mostrarClientes(const Sistema &s) {
    if (s.totalClientes == 0) {
        cout << "  No hay clientes registrados.\n";
        return;
    }
    cout << left << "  " << setw(5) << "No." << setw(26) << "Farmacia" << setw(11) << "Telefono"
         << "Direccion\n";
    cout << "  " << string(70, '-') << "\n";
    for (int i = 0; i < s.totalClientes; i++)
        cout << left << "  " << setw(5) << (i + 1) << setw(26) << cortar(s.clientes[i].nombreFarmacia, 25)
             << setw(11) << s.clientes[i].telefono << s.clientes[i].direccion << "\n";
}


// ====================================================================
//  9. REPORTES COMPARTIDOS
// ====================================================================

// Reporte 1: lotes ya vencidos o que vencen pronto, ordenados por fecha
void reporteVencimientos(const Sistema &s, const string &dueno) {
    titulo("REPORTE: LOTES VENCIDOS / POR VENCER (" + dueno + ")");
    string hoy = fechaHoy();
    int idx[MAX_LOTES];
    int n = 0;
    for (int i = 0; i < s.totalLotes; i++)
        if (s.lotes[i].dueno == dueno && s.lotes[i].stock > 0 &&
            (s.lotes[i].fecha < hoy || diasEntre(hoy, s.lotes[i].fecha) <= DIAS_ALERTA)) {
            idx[n] = i;
            n++;
        }
    if (n == 0) {
        cout << "  Ningun lote vencido ni por vencer en los proximos " << DIAS_ALERTA << " dias.\n";
        return;
    }
    ordenarIndicesPorFecha(s.lotes, idx, n);
    cout << left << "  " << setw(26) << "Producto" << setw(7) << "Lote" << setw(8) << "Stock"
         << setw(13) << "Vence" << "Situacion\n";
    cout << "  " << string(66, '-') << "\n";
    for (int k = 0; k < n; k++) {
        const Lote &l = s.lotes[idx[k]];
        string sit = (l.fecha < hoy) ? "VENCIDO" : ("vence en " + aTexto(diasEntre(hoy, l.fecha)) + " dias");
        cout << left << "  " << setw(26) << cortar(nombreProducto(s, l.cod), 25) << setw(7) << l.numLote
             << setw(8) << l.stock << setw(13) << l.fecha << sit << "\n";
    }
}

// Reporte 2: productos con poco stock, del que menos tiene al que mas (burbuja)
void reporteStockBajo(const Sistema &s, const string &dueno) {
    titulo("REPORTE: STOCK BAJO (menos de " + aTexto(STOCK_BAJO) + ") - " + dueno);
    string hoy = fechaHoy();
    int idx[MAX_PRODUCTOS];
    int st[MAX_PRODUCTOS];
    int n = 0;
    for (int i = 0; i < s.totalProductos; i++) {
        int v = stockVigente(s, s.productos[i].cod, dueno, hoy);
        if (v < STOCK_BAJO) {
            idx[n] = i;
            st[n] = v;
            n++;
        }
    }
    if (n == 0) {
        cout << "  Ningun producto con stock bajo.\n";
        return;
    }
    // ordenar de menor a mayor stock (burbuja), moviendo juntos stock e indice
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (st[j] > st[j + 1]) {
                int t = st[j]; st[j] = st[j + 1]; st[j + 1] = t;
                t = idx[j]; idx[j] = idx[j + 1]; idx[j + 1] = t;
            }
    cout << left << "  " << setw(10) << "Codigo" << setw(26) << "Producto" << "Stock vigente\n";
    cout << "  " << string(50, '-') << "\n";
    for (int k = 0; k < n; k++)
        cout << left << "  " << setw(10) << s.productos[idx[k]].cod << setw(26)
             << cortar(s.productos[idx[k]].nombre, 25) << st[k] << "\n";
}


// ====================================================================
//  10. MODULO CASA MEDICA: PRODUCTOS, CLIENTES Y USUARIOS
// ====================================================================

// Agrega un producto nuevo al catalogo junto con su PRIMER LOTE: numero de lote,
// cantidad en existencia y fecha de vencimiento (valida que el codigo no se repita).
// Para agregar mas lotes a un producto que ya existe se usa "Registrar lote".
void registrarProducto(Sistema &s) {
    titulo("REGISTRAR PRODUCTO");
    if (s.totalProductos >= MAX_PRODUCTOS) {
        cout << "  El catalogo esta lleno.\n";
        return;
    }
    if (s.totalLotes >= MAX_LOTES) {
        cout << "  No hay espacio para mas lotes.\n";
        return;
    }
    string cod = leerCodigo("  Codigo: ");
    if (buscarProducto(s, cod) != -1) {
        cout << "  Error: ya existe un producto con ese codigo.\n";
        return;
    }
    string nombre = leerTexto("  Nombre: ");
    float precio = leerPrecio("  Precio de venta (Q): ");
    cout << "  Datos del lote:\n";
    int num = leerEntero("  Numero de lote: ", 1, 99999);
    int cant = leerEntero("  Cantidad en existencia: ", 1, 1000000);
    string fecha = leerFechaVencimiento("  Fecha de vencimiento (AAAA-MM-DD): ");
    Producto &p = s.productos[s.totalProductos];
    s.totalProductos++;
    p.cod = cod;
    p.nombre = nombre;
    p.precio = precio;
    agregarStock(s, cod, num, fecha, cant, CASA);
    cout << "  Producto y lote registrados.\n";
}

// Agrega un lote a un producto que YA exista. Rechaza fechas ya vencidas.
void registrarLote(Sistema &s) {
    titulo("REGISTRAR LOTE");
    if (s.totalProductos == 0) {
        cout << "  Primero registre al menos un producto.\n";
        return;
    }
    if (s.totalLotes >= MAX_LOTES) {
        cout << "  No hay espacio para mas lotes.\n";
        return;
    }
    string cod = leerCodigo("  Codigo del producto: ");
    if (buscarProducto(s, cod) == -1) {
        cout << "  Error: el producto " << cod << " no existe.\n";
        return;
    }
    int num = leerEntero("  Numero de lote: ", 1, 99999);
    if (buscarLote(s, cod, num, CASA) != -1) {
        cout << "  Error: ese numero de lote ya existe para este producto.\n";
        return;
    }
    int cant = leerEntero("  Cantidad: ", 1, 1000000);
    string fecha = leerFechaVencimiento("  Fecha de vencimiento (AAAA-MM-DD): ");
    agregarStock(s, cod, num, fecha, cant, CASA);
    cout << "  Lote registrado para " << nombreProducto(s, cod) << ".\n";
}

// Submenu PRODUCTOS de la casa medica
void menuProductosCasa(Sistema &s) {
    int op;
    do {
        titulo("PRODUCTOS (CASA MEDICA)");
        cout << "  1. Ver inventario\n  2. Registrar producto nuevo\n  3. Registrar lote\n"
                "  4. Buscar producto\n  5. Volver\n";
        op = leerEntero("  Opcion: ", 1, 5);
        if (op == 1) mostrarInventario(s, CASA);
        else if (op == 2) registrarProducto(s);
        else if (op == 3) registrarLote(s);
        else if (op == 4) buscarProductos(s, CASA);
        guardarTodo(s);
        if (op != 5) pausar();
    } while (op != 5);
}

// Crea un usuario nuevo (pide nombre y contrasena) con el tipo indicado:
//   1 = casa medica, 2 = personal de farmacia, 3 = encargado de farmacia.
// Para tipo 2 y 3 queda ligado a la farmacia indicada.
// Devuelve true si se creo el usuario.
bool crearUsuarioNuevo(Sistema &s, const string &farmacia, int tipo) {
    if (s.totalUsuarios >= MAX_USUARIOS) {
        cout << "  No se pueden crear mas usuarios.\n";
        return false;
    }
    string nombre = leerTexto("  Nombre de usuario: ");
    if (buscarUsuario(s, nombre) != -1) {
        cout << "  Error: ese usuario ya existe.\n";
        return false;
    }
    string clave;
    while (true) {
        clave = leerTexto("  Contrasena (minimo 4 caracteres): ");
        if (clave.size() >= 4) break;
        cout << "  La contrasena es muy corta.\n";
    }
    Usuario &u = s.usuarios[s.totalUsuarios];
    s.totalUsuarios++;
    u.nombre = nombre;
    u.clave = clave;
    u.tipo = tipo;
    u.nomFarmacia = farmacia;
    if (tipo == 1) cout << "  Usuario de casa medica creado.\n";
    else if (tipo == 3) cout << "  Encargado creado para " << farmacia << ".\n";
    else cout << "  Usuario del personal creado para " << farmacia << ".\n";
    return true;
}

// Registra una farmacia como cliente (nombre, telefono, direccion) con boton de guardar.
// Al guardar ofrece crear de una vez el usuario ENCARGADO, que sera quien administre
// los usuarios del personal de esa farmacia.
void registrarCliente(Sistema &s) {
    titulo("REGISTRAR CLIENTE");
    if (s.totalClientes >= MAX_CLIENTES) {
        cout << "  Lista de clientes llena.\n";
        return;
    }
    string nombre = leerTexto("  Nombre de la farmacia: ");
    if (buscarCliente(s, nombre) != -1) {
        cout << "  Error: ese cliente ya esta registrado.\n";
        return;
    }
    string tel = leerTelefono("  Telefono (8 digitos): ");
    string dir = leerTexto("  Direccion: ");
    if (!confirmar("  Guardar cliente?")) {
        cout << "  Cliente no guardado.\n";
        return;
    }
    Cliente &c = s.clientes[s.totalClientes];
    s.totalClientes++;
    c.nombreFarmacia = nombre;
    c.telefono = tel;
    c.direccion = dir;
    cout << "  Cliente guardado.\n";
    if (confirmar("  Crear ahora el usuario ENCARGADO de esta farmacia?"))
        crearUsuarioNuevo(s, nombre, 3);
}

// Submenu CLIENTES
void menuClientes(Sistema &s) {
    int op;
    do {
        titulo("CLIENTES");
        cout << "  1. Registrar cliente\n  2. Ver todos los clientes\n  3. Volver\n";
        op = leerEntero("  Opcion: ", 1, 3);
        if (op == 1) registrarCliente(s);
        else if (op == 2) {
            titulo("LISTA DE CLIENTES");
            mostrarClientes(s);
        }
        guardarTodo(s);
        if (op != 3) pausar();
    } while (op != 3);
}

// Crea un usuario desde la casa medica: o bien otro usuario de casa medica, o bien
// el ENCARGADO de una farmacia (solo puede haber uno por farmacia). El encargado
// es quien despues crea los usuarios del personal de su farmacia.
void crearUsuario(Sistema &s) {
    titulo("CREAR USUARIO");
    cout << "  Tipo de usuario:\n  1. Casa medica\n  2. Encargado de una farmacia\n";
    int tipo = leerEntero("  Tipo: ", 1, 2);
    if (tipo == 1) {
        crearUsuarioNuevo(s, "", 1);
        return;
    }
    if (s.totalClientes == 0) {
        cout << "  Primero registre la farmacia como cliente.\n";
        return;
    }
    mostrarClientes(s);
    int c = leerEntero("  Farmacia del encargado (numero de la lista): ", 1, s.totalClientes);
    string farmacia = s.clientes[c - 1].nombreFarmacia;
    int e = buscarEncargado(s, farmacia);
    if (e != -1) {
        cout << "  Error: " << farmacia << " ya tiene encargado (" << s.usuarios[e].nombre << ").\n";
        return;
    }
    crearUsuarioNuevo(s, farmacia, 3);
}

// Convierte en encargado a un usuario que YA existe en una farmacia sin encargado
void nombrarEncargado(Sistema &s) {
    titulo("NOMBRAR ENCARGADO");
    if (s.totalClientes == 0) {
        cout << "  No hay farmacias registradas.\n";
        return;
    }
    mostrarClientes(s);
    int c = leerEntero("  Farmacia (numero de la lista): ", 1, s.totalClientes);
    string farmacia = s.clientes[c - 1].nombreFarmacia;
    int e = buscarEncargado(s, farmacia);
    if (e != -1) {
        cout << "  " << farmacia << " ya tiene encargado (" << s.usuarios[e].nombre << ").\n";
        return;
    }
    string nombre = leerTexto("  Usuario que sera el encargado: ");
    int u = buscarUsuario(s, nombre);
    if (u == -1 || s.usuarios[u].tipo == 1 || s.usuarios[u].nomFarmacia != farmacia) {
        cout << "  Ese usuario no existe o no pertenece a " << farmacia << ".\n";
        return;
    }
    if (confirmar("  Nombrar a " + nombre + " encargado de " + farmacia + "?")) {
        s.usuarios[u].tipo = 3;
        cout << "  Listo: " << nombre << " ahora es el encargado.\n";
    }
}

// Muestra los usuarios (sin mostrar las contrasenas)
void mostrarUsuarios(const Sistema &s) {
    titulo("USUARIOS");
    cout << left << "  " << setw(18) << "Usuario" << setw(14) << "Rol" << "Farmacia\n";
    cout << "  " << string(50, '-') << "\n";
    for (int i = 0; i < s.totalUsuarios; i++)
        cout << left << "  " << setw(18) << s.usuarios[i].nombre
             << setw(14) << nombreTipo(s.usuarios[i].tipo)
             << s.usuarios[i].nomFarmacia << "\n";
}

// Submenu USUARIOS
void menuUsuarios(Sistema &s) {
    int op;
    do {
        titulo("USUARIOS");
        cout << "  1. Crear usuario (casa medica o encargado de farmacia)\n  2. Ver usuarios\n"
                "  3. Nombrar encargado a un usuario existente\n  4. Volver\n";
        op = leerEntero("  Opcion: ", 1, 4);
        if (op == 1) crearUsuario(s);
        else if (op == 2) mostrarUsuarios(s);
        else if (op == 3) nombrarEncargado(s);
        guardarTodo(s);
        if (op != 4) pausar();
    } while (op != 4);
}

// ====================================================================
//  11. MODULO ENVIOS Y REPARTIDORES
//  Hay 3 repartidores. Un repartidor esta ocupado mientras tenga un
//  envio "En camino". Si los 3 estan ocupados, el envio queda "Pendiente".
// ====================================================================

// Devuelve la posicion del envio en camino que tiene el repartidor r (o -1 si esta libre)
int envioDeRepartidor(const Sistema &s, int r) {
    for (int i = 0; i < s.totalEnvios; i++)
        if (s.envios[i].estado == "En camino" && s.envios[i].repartidor == r) return i;
    return -1;
}

// Devuelve el numero del primer repartidor libre (1 a 3), o 0 si todos estan ocupados
int repartidorLibre(const Sistema &s) {
    for (int r = 1; r <= NUM_REPARTIDORES; r++)
        if (envioDeRepartidor(s, r) == -1) return r;
    return 0;
}

// Cuenta los envios que estan en cierto estado
int contarEnvios(const Sistema &s, const string &estado) {
    int n = 0;
    for (int i = 0; i < s.totalEnvios; i++)
        if (s.envios[i].estado == estado) n++;
    return n;
}

// Intenta asignar un repartidor libre a un envio y calcula el tiempo estimado.
// Tiempo estimado = 30 min base + 10 min por cada producto distinto del pedido.
// Devuelve false si los 3 repartidores estan ocupados.
bool asignarRepartidor(Sistema &s, int idxEnvio) {
    int r = repartidorLibre(s);
    if (r == 0) return false;
    Envio &e = s.envios[idxEnvio];
    int numProductos = 0;
    int idxPre = -1;
    for (int i = 0; i < s.totalPresupuestos; i++)
        if (s.presupuestos[i].id == e.idPresupuesto) idxPre = i;
    if (idxPre != -1) numProductos = s.presupuestos[idxPre].numItems;
    e.repartidor = r;
    e.estado = "En camino";
    e.tiempo = 30 + 10 * numProductos;
    return true;
}

// Muestra cual repartidor esta libre y cual ocupado
void mostrarRepartidores(const Sistema &s) {
    titulo("REPARTIDORES");
    for (int r = 1; r <= NUM_REPARTIDORES; r++) {
        int e = envioDeRepartidor(s, r);
        cout << "  Repartidor " << r << ": ";
        if (e == -1) cout << "LIBRE\n";
        else cout << "OCUPADO - envio #" << s.envios[e].id << " para " << s.envios[e].cliente
                  << " (aprox. " << s.envios[e].tiempo << " min)\n";
    }
    cout << "  Envios pendientes de repartidor: " << contarEnvios(s, "Pendiente") << "\n";
}

// Muestra la tabla de todos los envios
void mostrarEnvios(const Sistema &s) {
    titulo("LISTA DE ENVIOS");
    if (s.totalEnvios == 0) {
        cout << "  No hay envios registrados.\n";
        return;
    }
    cout << left << "  " << setw(6) << "Envio" << setw(8) << "Presup." << setw(24) << "Cliente"
         << setw(12) << "Repartidor" << setw(12) << "Estado" << "Tiempo est.\n";
    cout << "  " << string(72, '-') << "\n";
    for (int i = 0; i < s.totalEnvios; i++) {
        const Envio &e = s.envios[i];
        string rep = (e.repartidor == 0) ? "-" : aTexto(e.repartidor);
        string t = (e.estado == "En camino") ? (aTexto(e.tiempo) + " min") : "-";
        cout << left << "  " << setw(6) << e.id << setw(8) << e.idPresupuesto << setw(24)
             << cortar(e.cliente, 23) << setw(12) << rep << setw(12) << e.estado << t << "\n";
    }
}

// Busca el primer envio pendiente y le asigna el repartidor que quedo libre (si hay)
void asignarSiguientePendiente(Sistema &s) {
    int sig = -1;
    for (int i = 0; i < s.totalEnvios; i++)
        if (s.envios[i].estado == "Pendiente") {
            sig = i;
            break;
        }
    if (sig != -1 && asignarRepartidor(s, sig))
        cout << "  Envio pendiente #" << s.envios[sig].id << " asignado al repartidor "
             << s.envios[sig].repartidor << " (aprox. " << s.envios[sig].tiempo << " min).\n";
}

// Marca un envio como entregado. Esto hace tres cosas:
//   1) pasa el stock al inventario de la farmacia
//   2) libera al repartidor
//   3) asigna automaticamente el siguiente envio pendiente (si hay)
void marcarEntregado(Sistema &s) {
    titulo("MARCAR ENVIO COMO ENTREGADO");
    if (s.totalEnvios == 0) {
        cout << "  No hay envios registrados.\n";
        return;
    }
    int id = leerEntero("  Numero de envio: ", 1, 999999);
    int idx = -1;
    for (int i = 0; i < s.totalEnvios; i++)
        if (s.envios[i].id == id) idx = i;
    if (idx == -1) {
        cout << "  Error: el envio #" << id << " no existe.\n";
        return;
    }
    Envio &e = s.envios[idx];
    if (e.estado == "Entregado") {
        cout << "  Este envio ya fue entregado.\n";
        return;
    }
    if (e.estado == "Cancelado") {
        cout << "  Este envio fue cancelado.\n";
        return;
    }
    if (e.estado == "Pendiente") {
        cout << "  Este envio aun no tiene repartidor, no puede entregarse.\n";
        return;
    }
    if (s.totalLotes + e.numLineas > MAX_LOTES) {
        cout << "  No hay espacio en el sistema para recibir los lotes.\n";
        return;
    }
    // 1) transferir el stock a la farmacia
    for (int i = 0; i < e.numLineas; i++)
        agregarStock(s, e.lineas[i].cod, e.lineas[i].numLote, e.lineas[i].fecha,
                     e.lineas[i].cantidad, e.cliente);
    // 2) liberar al repartidor
    int repLiberado = e.repartidor;
    e.estado = "Entregado";
    cout << "  Envio #" << e.id << " entregado. El inventario de " << e.cliente << " fue actualizado.\n";
    cout << "  El repartidor " << repLiberado << " ahora esta libre.\n";

    // 3) darle el repartidor libre al primer envio pendiente
    asignarSiguientePendiente(s);
}

// Cancela un envio que todavia no se entrego: el stock regresa al inventario de la
// casa medica, se libera el repartidor y el presupuesto y la solicitud quedan cancelados.
void cancelarEnvio(Sistema &s) {
    titulo("CANCELAR ENVIO");
    if (s.totalEnvios == 0) {
        cout << "  No hay envios registrados.\n";
        return;
    }
    int id = leerEntero("  Numero de envio: ", 1, 999999);
    int idx = -1;
    for (int i = 0; i < s.totalEnvios; i++)
        if (s.envios[i].id == id) idx = i;
    if (idx == -1) {
        cout << "  Error: el envio #" << id << " no existe.\n";
        return;
    }
    Envio &e = s.envios[idx];
    if (e.estado == "Entregado") {
        cout << "  Este envio ya fue entregado: el producto ya esta en la farmacia.\n";
        return;
    }
    if (e.estado == "Cancelado") {
        cout << "  Este envio ya estaba cancelado.\n";
        return;
    }
    if (!confirmar("  Cancelar el envio #" + aTexto(e.id) + " de " + e.cliente + "?")) {
        cout << "  No se cancelo.\n";
        return;
    }
    // 1) regresar el stock a la casa medica (a los mismos lotes de donde salio)
    for (int i = 0; i < e.numLineas; i++)
        agregarStock(s, e.lineas[i].cod, e.lineas[i].numLote, e.lineas[i].fecha,
                     e.lineas[i].cantidad, CASA);
    // 2) cerrar el envio y, si tenia repartidor, dejarlo libre
    bool teniaRepartidor = (e.estado == "En camino");
    e.estado = "Cancelado";
    e.repartidor = 0;
    e.tiempo = 0;
    // 3) cancelar el presupuesto y la solicitud de donde nacio
    for (int i = 0; i < s.totalPresupuestos; i++) {
        if (s.presupuestos[i].id == e.idPresupuesto) {
            s.presupuestos[i].estado = "Cancelado";
            for (int k = 0; k < s.totalSolicitudes; k++)
                if (s.solicitudes[k].id == s.presupuestos[i].idSolicitud)
                    s.solicitudes[k].estado = "Cancelada";
        }
    }
    cout << "  Envio #" << e.id << " cancelado. El stock regreso al inventario de la casa medica.\n";
    // 4) si se libero un repartidor, atender al siguiente envio pendiente
    if (teniaRepartidor) asignarSiguientePendiente(s);
}

// Submenu ENVIOS
void menuEnvios(Sistema &s) {
    int op;
    do {
        titulo("ENVIOS");
        cout << "  1. Ver repartidores\n  2. Ver lista de envios\n  3. Marcar envio como entregado\n"
                "  4. Cancelar envio\n  5. Volver\n";
        op = leerEntero("  Opcion: ", 1, 5);
        if (op == 1) mostrarRepartidores(s);
        else if (op == 2) mostrarEnvios(s);
        else if (op == 3) marcarEntregado(s);
        else if (op == 4) cancelarEnvio(s);
        guardarTodo(s);
        if (op != 5) pausar();
    } while (op != 5);
}


// ====================================================================
//  12. MODULO PRESUPUESTOS Y SOLICITUDES (Casa Medica)
//  Flujo: la farmacia manda una SOLICITUD -> la casa medica genera un
//  PRESUPUESTO -> desde el presupuesto se hace el ENVIO.
// ====================================================================

// Crea el envio de un presupuesto. Verifica el stock, lo retira del inventario
// de la casa medica y trata de asignar un repartidor. Si no hay, queda Pendiente.
void hacerEnvio(Sistema &s, int idxPre) {
    Presupuesto &p = s.presupuestos[idxPre];
    if (p.estado == "Cancelado") {
        cout << "  Este presupuesto fue cancelado.\n";
        return;
    }
    if (p.estado != "Sin envio") {
        cout << "  Este presupuesto ya tiene un envio.\n";
        return;
    }
    if (s.totalEnvios >= MAX_ENVIOS) {
        cout << "  No se pueden registrar mas envios.\n";
        return;
    }
    string hoy = fechaHoy();
    bool ok = true;
    int lineasNecesarias = 0;
    // revisar que alcance el stock de TODOS los productos antes de tocar nada
    for (int i = 0; i < p.numItems; i++) {
        int disp = stockVigente(s, p.items[i].cod, CASA, hoy);
        if (disp < p.items[i].cantidad) {
            cout << "  Stock insuficiente de " << nombreProducto(s, p.items[i].cod)
                 << ": pedido " << p.items[i].cantidad << ", disponible " << disp << "\n";
            ok = false;
        }
        lineasNecesarias += lotesVigentes(s, p.items[i].cod, CASA, hoy);
    }
    if (lineasNecesarias > MAX_LINEAS) {
        cout << "  El pedido involucra demasiados lotes para un solo envio.\n";
        ok = false;
    }
    if (!ok) {
        cout << "  No se pudo crear el envio.\n";
        return;
    }
    // crear el envio y retirar el stock (primero los lotes que vencen antes)
    Envio &e = s.envios[s.totalEnvios];
    e.id = s.totalEnvios + 1;
    e.idPresupuesto = p.id;
    e.cliente = p.cliente;
    e.repartidor = 0;
    e.estado = "Pendiente";
    e.tiempo = 0;
    e.numLineas = 0;
    for (int i = 0; i < p.numItems; i++)
        descontarStock(s, p.items[i].cod, CASA, p.items[i].cantidad, e.lineas, e.numLineas);
    s.totalEnvios++;
    p.estado = "Con envio";
    cout << "  Envio #" << e.id << " creado. El stock fue retirado del inventario de la casa medica.\n";
    if (asignarRepartidor(s, s.totalEnvios - 1))
        cout << "  Asignado al repartidor " << e.repartidor << ". Tiempo estimado: " << e.tiempo << " min.\n";
    else
        cout << "  Los " << NUM_REPARTIDORES << " repartidores estan ocupados: el envio queda PENDIENTE.\n";
}

// Cancela un presupuesto que todavia no tiene envio; la solicitud tambien queda cancelada
void cancelarPresupuesto(Sistema &s, int idx) {
    Presupuesto &p = s.presupuestos[idx];
    if (p.estado != "Sin envio") {
        cout << "  Solo se puede cancelar un presupuesto sin envio. Si ya tiene envio, cancele el envio.\n";
        return;
    }
    if (!confirmar("  Cancelar el presupuesto #" + aTexto(p.id) + "?")) {
        cout << "  No se cancelo.\n";
        return;
    }
    p.estado = "Cancelado";
    for (int k = 0; k < s.totalSolicitudes; k++)
        if (s.solicitudes[k].id == p.idSolicitud) s.solicitudes[k].estado = "Cancelada";
    cout << "  Presupuesto cancelado.\n";
}

// Muestra el detalle de un presupuesto (productos, cantidades, total) y ofrece hacer el envio
void verPresupuesto(Sistema &s, int idx) {
    Presupuesto &p = s.presupuestos[idx];
    titulo("PRESUPUESTO #" + aTexto(p.id));
    cout << "  Fecha:   " << p.fecha << "\n  Cliente: " << p.cliente << "\n  Estado:  " << p.estado << "\n\n";
    mostrarItems(s, p.items, p.numItems, true);
    cout << "\n  TOTAL: " << dinero(p.total) << "\n";
    if (p.estado == "Sin envio") {
        cout << "\n  1. Hacer envio\n  2. Cancelar presupuesto\n  3. Volver\n";
        int op = leerEntero("  Opcion: ", 1, 3);
        if (op == 1) hacerEnvio(s, idx);
        else if (op == 2) cancelarPresupuesto(s, idx);
    } else if (p.estado == "Con envio") {
        cout << "\n  (para cancelarlo use Envios > Cancelar envio)\n";
    }
}

// Lista de presupuestos: numero, fecha y total. Se puede abrir uno para ver su detalle.
void menuPresupuestos(Sistema &s) {
    int op;
    do {
        titulo("PRESUPUESTOS");
        if (s.totalPresupuestos == 0) {
            cout << "  No hay presupuestos. Se generan desde las solicitudes de las farmacias.\n";
            pausar();
            return;
        }
        cout << left << "  " << setw(6) << "No." << setw(13) << "Fecha" << setw(24) << "Cliente"
             << setw(14) << "Total" << "Estado\n";
        cout << "  " << string(66, '-') << "\n";
        for (int i = 0; i < s.totalPresupuestos; i++) {
            const Presupuesto &p = s.presupuestos[i];
            cout << left << "  " << setw(6) << p.id << setw(13) << p.fecha << setw(24)
                 << cortar(p.cliente, 23) << setw(14) << dinero(p.total) << p.estado << "\n";
        }
        cout << "\n  Escriba el numero del presupuesto para abrirlo (0 = volver)\n";
        op = leerEntero("  Presupuesto: ", 0, 999999);
        if (op != 0) {
            int idx = -1;
            for (int i = 0; i < s.totalPresupuestos; i++)
                if (s.presupuestos[i].id == op) idx = i;
            if (idx == -1) cout << "  Ese presupuesto no existe.\n";
            else verPresupuesto(s, idx);
            guardarTodo(s);
            pausar();
        }
    } while (op != 0);
}

// Convierte una solicitud de farmacia en un presupuesto (calcula el total con los precios)
void generarPresupuesto(Sistema &s, int idxSol) {
    Solicitud &sol = s.solicitudes[idxSol];
    if (sol.estado != "Nueva") {
        cout << "  Esta solicitud ya no esta disponible (estado: " << sol.estado << ").\n";
        return;
    }
    if (s.totalPresupuestos >= MAX_PRESUPUESTOS) {
        cout << "  No se pueden generar mas presupuestos.\n";
        return;
    }
    string hoy = fechaHoy();
    bool ok = true;
    for (int i = 0; i < sol.numItems; i++) {
        int disp = stockVigente(s, sol.items[i].cod, CASA, hoy);
        if (disp < sol.items[i].cantidad) {
            cout << "  Stock insuficiente de " << nombreProducto(s, sol.items[i].cod)
                 << ": solicitado " << sol.items[i].cantidad << ", disponible " << disp << "\n";
            ok = false;
        }
    }
    if (!ok) {
        cout << "  No se puede generar el presupuesto.\n";
        return;
    }
    // elegir para que cliente es el presupuesto
    cout << "\n  Seleccione el cliente del presupuesto:\n";
    mostrarClientes(s);
    string cliente = sol.farmacia;
    if (s.totalClientes > 0) {
        int c = leerEntero("  Numero de cliente (0 = " + sol.farmacia + "): ", 0, s.totalClientes);
        if (c != 0) cliente = s.clientes[c - 1].nombreFarmacia;
    }
    int idxCli = buscarCliente(s, cliente);
    if (idxCli == -1) {
        cout << "  Error: la farmacia " << cliente << " no esta registrada como cliente.\n";
        return;
    }
    cliente = s.clientes[idxCli].nombreFarmacia;
    // crear el presupuesto
    Presupuesto &p = s.presupuestos[s.totalPresupuestos];
    p.id = s.totalPresupuestos + 1;
    p.fecha = hoy;
    p.cliente = cliente;
    p.idSolicitud = sol.id;
    p.estado = "Sin envio";
    p.numItems = sol.numItems;
    p.total = 0;
    for (int i = 0; i < sol.numItems; i++) {
        p.items[i] = sol.items[i];
        int pr = buscarProducto(s, sol.items[i].cod);
        if (pr != -1) p.total += s.productos[pr].precio * sol.items[i].cantidad;
    }
    s.totalPresupuestos++;
    sol.estado = "Atendida";
    cout << "  Presupuesto #" << p.id << " generado por " << dinero(p.total) << " para " << cliente << ".\n";
}

// Cuenta las solicitudes que todavia no se han atendido (para la notificacion)
int contarSolicitudesNuevas(const Sistema &s) {
    int n = 0;
    for (int i = 0; i < s.totalSolicitudes; i++)
        if (s.solicitudes[i].estado == "Nueva") n++;
    return n;
}

// Cancela un pedido de farmacia que todavia no ha sido atendido (estado "Nueva").
// La usan la farmacia (cancela su propio pedido) y la casa medica (rechaza un pedido).
void cancelarSolicitud(Sistema &s, int idx) {
    Solicitud &sol = s.solicitudes[idx];
    if (sol.estado != "Nueva") {
        cout << "  Solo se puede cancelar un pedido que aun no ha sido atendido.\n";
        return;
    }
    if (!confirmar("  Cancelar el pedido #" + aTexto(sol.id) + "?")) {
        cout << "  No se cancelo.\n";
        return;
    }
    sol.estado = "Cancelada";
    cout << "  Pedido cancelado.\n";
}

// Lista de solicitudes que mandaron las farmacias. Desde aqui se genera el presupuesto.
void menuSolicitudes(Sistema &s) {
    int op;
    do {
        titulo("SOLICITUDES DE LAS FARMACIAS");
        if (s.totalSolicitudes == 0) {
            cout << "  No hay solicitudes.\n";
            pausar();
            return;
        }
        cout << left << "  " << setw(6) << "No." << setw(26) << "Farmacia" << setw(13) << "Fecha"
             << "Estado\n";
        cout << "  " << string(56, '-') << "\n";
        for (int i = 0; i < s.totalSolicitudes; i++)
            cout << left << "  " << setw(6) << s.solicitudes[i].id << setw(26)
                 << cortar(s.solicitudes[i].farmacia, 25) << setw(13) << s.solicitudes[i].fecha
                 << s.solicitudes[i].estado << "\n";
        cout << "\n  Escriba el numero de la solicitud para verla (0 = volver)\n";
        op = leerEntero("  Solicitud: ", 0, 999999);
        if (op != 0) {
            int idx = -1;
            for (int i = 0; i < s.totalSolicitudes; i++)
                if (s.solicitudes[i].id == op) idx = i;
            if (idx == -1) {
                cout << "  Esa solicitud no existe.\n";
            } else {
                Solicitud &sol = s.solicitudes[idx];
                titulo("SOLICITUD #" + aTexto(sol.id) + " - " + sol.farmacia);
                mostrarItems(s, sol.items, sol.numItems, false);
                if (sol.estado == "Nueva") {
                    cout << "\n  1. Generar presupuesto\n  2. Cancelar pedido\n  3. Volver\n";
                    int accion = leerEntero("  Opcion: ", 1, 3);
                    if (accion == 1) generarPresupuesto(s, idx);
                    else if (accion == 2) cancelarSolicitud(s, idx);
                } else {
                    cout << "\n  (solicitud " << sol.estado << ")\n";
                }
            }
            guardarTodo(s);
            pausar();
        }
    } while (op != 0);
}

// ====================================================================
//  13. REPORTES Y MENU PRINCIPAL DE LA CASA MEDICA
// ====================================================================

// Reporte 3: resumen general de solicitudes, presupuestos y envios
void reporteResumenCasa(const Sistema &s) {
    titulo("REPORTE: RESUMEN DE OPERACIONES");
    int nuevas = contarSolicitudesNuevas(s);
    float montoTotal = 0;
    int sinEnvio = 0;
    int solCanceladas = 0;
    for (int i = 0; i < s.totalPresupuestos; i++) {
        if (s.presupuestos[i].estado != "Cancelado") montoTotal += s.presupuestos[i].total;
        if (s.presupuestos[i].estado == "Sin envio") sinEnvio++;
    }
    for (int i = 0; i < s.totalSolicitudes; i++)
        if (s.solicitudes[i].estado == "Cancelada") solCanceladas++;
    cout << "  Solicitudes recibidas:      " << s.totalSolicitudes << " (nuevas: " << nuevas
         << ", canceladas: " << solCanceladas << ")\n";
    cout << "  Presupuestos generados:     " << s.totalPresupuestos << " (sin envio: " << sinEnvio << ")\n";
    cout << "  Monto presupuestado (sin cancelados): " << dinero(montoTotal) << "\n";
    cout << "  Envios pendientes:          " << contarEnvios(s, "Pendiente") << "\n";
    cout << "  Envios en camino:           " << contarEnvios(s, "En camino") << "\n";
    cout << "  Envios entregados:          " << contarEnvios(s, "Entregado") << "\n";
    cout << "  Envios cancelados:          " << contarEnvios(s, "Cancelado") << "\n";
    cout << "  Clientes registrados:       " << s.totalClientes << "\n";
}

// Submenu REPORTES de la casa medica
void menuReportesCasa(Sistema &s) {
    int op;
    do {
        titulo("REPORTES (CASA MEDICA)");
        cout << "  1. Lotes vencidos / por vencer\n  2. Productos con stock bajo\n"
                "  3. Resumen de solicitudes, presupuestos y envios\n  4. Volver\n";
        op = leerEntero("  Opcion: ", 1, 4);
        if (op == 1) reporteVencimientos(s, CASA);
        else if (op == 2) reporteStockBajo(s, CASA);
        else if (op == 3) reporteResumenCasa(s);
        if (op != 4) pausar();
    } while (op != 4);
}

// MENU PRINCIPAL DE LA CASA MEDICA: productos, solicitudes, presupuestos,
// envios, clientes, usuarios y reportes. Muestra una notificacion si hay
// solicitudes nuevas de las farmacias.
void menuCasaMedica(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("CASA MEDICA - Usuario: " + u.nombre);
        int nuevas = contarSolicitudesNuevas(s);
        if (nuevas > 0)
            cout << "  *** NOTIFICACION: " << nuevas << " solicitud(es) nueva(s) de farmacias ***\n";
        cout << "  1. Productos\n  2. Solicitudes de farmacias\n  3. Presupuestos\n  4. Envios\n"
                "  5. Clientes\n  6. Usuarios\n  7. Reportes\n  8. Cerrar sesion\n";
        op = leerEntero("  Opcion: ", 1, 8);
        switch (op) {
            case 1: menuProductosCasa(s); break;
            case 2: menuSolicitudes(s); break;
            case 3: menuPresupuestos(s); break;
            case 4: menuEnvios(s); break;
            case 5: menuClientes(s); break;
            case 6: menuUsuarios(s); break;
            case 7: menuReportesCasa(s); break;
            default: cout << "  Sesion cerrada.\n";
        }
        guardarTodo(s);
    } while (op != 8);
}


// ====================================================================
//  14. MODULO FARMACIA (venta al publico)
//  La farmacia NO tiene presupuestos de compra ni clientes. Tiene: productos,
//  pedido a la casa medica, ventas (cada una con su vendedor), usuarios para
//  su personal y reportes.
// ====================================================================

// La farmacia arma su lista de productos necesarios y la manda a la casa medica.
// A la casa medica le aparece como "solicitud nueva".
void crearSolicitud(Sistema &s, const Usuario &u) {
    titulo("NUEVO PEDIDO A LA CASA MEDICA");
    if (s.totalSolicitudes >= MAX_SOLICITUDES) {
        cout << "  No se pueden enviar mas pedidos.\n";
        return;
    }
    cout << "  Productos disponibles en la casa medica:\n\n";
    mostrarCatalogoConStock(s, CASA);
    string hoy = fechaHoy();
    Solicitud nueva;
    nueva.id = s.totalSolicitudes + 1;
    nueva.farmacia = u.nomFarmacia;
    nueva.fecha = hoy;
    nueva.estado = "Nueva";
    nueva.numItems = 0;
    bool seguir = true;
    while (seguir && nueva.numItems < MAX_ITEMS) {
        string cod = leerCodigo("\n  Codigo del producto (0 = terminar, ? = buscar por nombre): ");
        if (cod == "0") {
            seguir = false;
        } else if (cod == "?") {
            buscarProductos(s, CASA);
        } else if (buscarProducto(s, cod) == -1) {
            cout << "  Ese producto no existe en el catalogo.\n";
        } else if (buscarItem(nueva.items, nueva.numItems, cod) != -1) {
            cout << "  Ese producto ya esta en el pedido.\n";
        } else {
            int disp = stockVigente(s, cod, CASA, hoy);
            if (disp <= 0) {
                cout << "  La casa medica no tiene stock de ese producto.\n";
            } else {
                int cant = leerEntero("  Cantidad (maximo " + aTexto(disp) + "): ", 1, disp);
                nueva.items[nueva.numItems].cod = cod;
                nueva.items[nueva.numItems].cantidad = cant;
                nueva.numItems++;
                cout << "  Agregado.\n";
            }
        }
    }
    if (nueva.numItems == 0) {
        cout << "  Pedido vacio: no se envio.\n";
        return;
    }
    cout << "\n  Resumen del pedido:\n";
    mostrarItems(s, nueva.items, nueva.numItems, false);
    if (!confirmar("\n  Enviar pedido a la casa medica?")) {
        cout << "  Pedido cancelado.\n";
        return;
    }
    s.solicitudes[s.totalSolicitudes] = nueva;
    s.totalSolicitudes++;
    cout << "  Pedido #" << nueva.id << " enviado. La casa medica recibio la notificacion.\n";
}

// Muestra los pedidos que ha hecho esta farmacia y en que estado estan
void verMisPedidos(const Sistema &s, const Usuario &u) {
    titulo("MIS PEDIDOS");
    int n = 0;
    cout << left << "  " << setw(6) << "No." << setw(13) << "Fecha" << setw(11) << "Productos"
         << "Estado\n";
    cout << "  " << string(44, '-') << "\n";
    for (int i = 0; i < s.totalSolicitudes; i++) {
        if (s.solicitudes[i].farmacia == u.nomFarmacia) {
            cout << left << "  " << setw(6) << s.solicitudes[i].id << setw(13) << s.solicitudes[i].fecha
                 << setw(11) << s.solicitudes[i].numItems << s.solicitudes[i].estado << "\n";
            n++;
        }
    }
    if (n == 0) cout << "  Aun no ha enviado pedidos.\n";
}

// La farmacia cancela uno de SUS pedidos, siempre que la casa medica no lo haya atendido
void cancelarPedidoFarmacia(Sistema &s, const Usuario &u) {
    titulo("CANCELAR PEDIDO");
    verMisPedidos(s, u);
    int id = leerEntero("\n  Numero del pedido a cancelar (0 = volver): ", 0, 999999);
    if (id == 0) return;
    int idx = -1;
    for (int i = 0; i < s.totalSolicitudes; i++)
        if (s.solicitudes[i].id == id && s.solicitudes[i].farmacia == u.nomFarmacia) idx = i;
    if (idx == -1) {
        cout << "  Ese pedido no existe.\n";
    } else if (s.solicitudes[idx].estado == "Nueva") {
        cancelarSolicitud(s, idx);
    } else if (s.solicitudes[idx].estado == "Atendida") {
        cout << "  La casa medica ya atendio este pedido. Para cancelarlo comuniquese con ella.\n";
    } else {
        cout << "  Este pedido ya estaba cancelado.\n";
    }
}

// Submenu PEDIDO A CASA MEDICA
void menuPedidosFarmacia(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("PEDIDO A CASA MEDICA");
        cout << "  1. Crear pedido nuevo\n  2. Ver mis pedidos\n  3. Cancelar un pedido\n  4. Volver\n";
        op = leerEntero("  Opcion: ", 1, 4);
        if (op == 1) crearSolicitud(s, u);
        else if (op == 2) verMisPedidos(s, u);
        else if (op == 3) cancelarPedidoFarmacia(s, u);
        guardarTodo(s);
        if (op != 4) pausar();
    } while (op != 4);
}

// ---- VENTAS: una venta (presupuesto de venta) puede llevar varios productos ----

// Numero que le toca a la proxima venta de esta farmacia (cada farmacia lleva su numeracion)
int siguienteNumVenta(const Sistema &s, const string &farmacia) {
    int mayor = 0;
    for (int i = 0; i < s.totalVentas; i++)
        if (s.ventas[i].farmacia == farmacia && s.ventas[i].idVenta > mayor)
            mayor = s.ventas[i].idVenta;
    return mayor + 1;
}

// Registra una venta al publico. Se pueden agregar varios productos; al final se
// muestra el resumen y, si se confirma, se descuenta del inventario de la farmacia
// y se guarda QUIEN vendio. Valida que el producto exista, que haya stock vigente
// (sin lotes vencidos) y que no se venda mas de lo disponible.
void registrarVenta(Sistema &s, const Usuario &u) {
    titulo("REGISTRAR VENTA");
    string hoy = fechaHoy();
    cout << "  Productos disponibles para la venta:\n\n";
    mostrarCatalogoConStock(s, u.nomFarmacia);
    Item carrito[MAX_ITEMS];   // productos de esta venta
    int nCarrito = 0;
    bool seguir = true;
    while (seguir && nCarrito < MAX_ITEMS) {
        string cod = leerCodigo("\n  Codigo del producto (0 = terminar, ? = buscar por nombre): ");
        if (cod == "0") {
            seguir = false;
        } else if (cod == "?") {
            buscarProductos(s, u.nomFarmacia);
        } else if (buscarProducto(s, cod) == -1) {
            cout << "  Error: ese producto no existe.\n";
        } else if (buscarItem(carrito, nCarrito, cod) != -1) {
            cout << "  Ese producto ya esta en la venta.\n";
        } else {
            int vigente = stockVigente(s, cod, u.nomFarmacia, hoy);
            if (vigente <= 0) {
                cout << "  No hay existencias vigentes de ese producto (sin stock o lotes vencidos).\n";
            } else {
                int cant = leerEntero("  Cantidad (maximo " + aTexto(vigente) + "): ", 1, vigente);
                carrito[nCarrito].cod = cod;
                carrito[nCarrito].cantidad = cant;
                nCarrito++;
                cout << "  Agregado.\n";
            }
        }
    }
    if (nCarrito == 0) {
        cout << "  Venta vacia: no se registro.\n";
        return;
    }
    // resumen con precios y total
    float total = 0;
    int lineasNecesarias = 0;
    for (int i = 0; i < nCarrito; i++) {
        int p = buscarProducto(s, carrito[i].cod);
        if (p != -1) total += s.productos[p].precio * carrito[i].cantidad;
        lineasNecesarias += lotesVigentes(s, carrito[i].cod, u.nomFarmacia, hoy);
    }
    cout << "\n  Resumen de la venta:\n";
    mostrarItems(s, carrito, nCarrito, true);
    cout << "\n  TOTAL: " << dinero(total) << "\n";
    if (s.totalVentas + lineasNecesarias > MAX_VENTAS) {
        cout << "  El registro de ventas esta lleno.\n";
        return;
    }
    if (!confirmar("\n  Confirmar venta?")) {
        cout << "  Venta cancelada.\n";
        return;
    }
    // descontar cada producto (primero el lote que vence antes) y guardar las lineas
    int idVenta = siguienteNumVenta(s, u.nomFarmacia);
    float totalVenta = 0;
    int guardadas = 0;
    for (int k = 0; k < nCarrito; k++) {
        LineaEnvio lineas[MAX_LINEAS];
        int nLineas = 0;
        int p = buscarProducto(s, carrito[k].cod);
        if (!descontarStock(s, carrito[k].cod, u.nomFarmacia, carrito[k].cantidad, lineas, nLineas)) {
            cout << "  No se pudo vender " << nombreProducto(s, carrito[k].cod) << ".\n";
            continue;
        }
        for (int i = 0; i < nLineas; i++) {
            Venta &v = s.ventas[s.totalVentas];
            v.idVenta = idVenta;
            v.farmacia = u.nomFarmacia;
            v.fecha = hoy;
            v.vendedor = u.nombre;
            v.estado = "Activa";
            v.cod = carrito[k].cod;
            v.numLote = lineas[i].numLote;
            v.cantidad = lineas[i].cantidad;
            v.total = s.productos[p].precio * lineas[i].cantidad;
            totalVenta += v.total;
            s.totalVentas++;
            guardadas++;
        }
    }
    if (guardadas == 0) cout << "  No se registro ninguna venta.\n";
    else cout << "  Venta #" << idVenta << " registrada por " << u.nombre
              << ". Total: " << dinero(totalVenta) << "\n";
}

// Junta las lineas de venta de una farmacia en ventas completas (una por cada idVenta).
// Llena los arreglos con numero, fecha, vendedor, estado y total de cada venta.
// Devuelve cuantas ventas hay.
int agruparVentas(const Sistema &s, const string &farmacia, int ids[], string fechas[],
                  string vendedores[], float totales[], string estados[]) {
    int n = 0;
    for (int i = 0; i < s.totalVentas; i++) {
        const Venta &v = s.ventas[i];
        if (v.farmacia != farmacia) continue;
        int pos = -1;
        for (int k = 0; k < n; k++)
            if (ids[k] == v.idVenta) pos = k;
        if (pos == -1) {   // primera linea de esta venta
            pos = n;
            ids[n] = v.idVenta;
            fechas[n] = v.fecha;
            vendedores[n] = v.vendedor;
            estados[n] = v.estado;
            totales[n] = 0;
            n++;
        }
        totales[pos] += v.total;
    }
    return n;
}

// Cancela una venta completa: cada producto regresa al lote del que salio y la venta
// queda marcada como "Cancelada" (no se borra, para conservar el historial).
// El encargado puede cancelar cualquier venta; el personal solo las suyas.
void cancelarVenta(Sistema &s, const Usuario &u, int idVenta) {
    bool existe = false;
    string vendedor = "";
    string estado = "";
    for (int i = 0; i < s.totalVentas; i++) {
        const Venta &v = s.ventas[i];
        if (v.farmacia == u.nomFarmacia && v.idVenta == idVenta) {
            existe = true;
            vendedor = v.vendedor;
            estado = v.estado;
        }
    }
    if (!existe) {
        cout << "  Esa venta no existe.\n";
        return;
    }
    if (estado == "Cancelada") {
        cout << "  Esta venta ya estaba cancelada.\n";
        return;
    }
    if (u.tipo != 3 && vendedor != u.nombre) {
        cout << "  Solo el encargado puede cancelar ventas de otro vendedor.\n";
        return;
    }
    if (!confirmar("  Cancelar la venta #" + aTexto(idVenta) + "? Los productos regresan al inventario.")) {
        cout << "  No se cancelo.\n";
        return;
    }
    for (int i = 0; i < s.totalVentas; i++) {
        Venta &v = s.ventas[i];
        if (v.farmacia == u.nomFarmacia && v.idVenta == idVenta) {
            int l = buscarLote(s, v.cod, v.numLote, u.nomFarmacia);
            if (l != -1) s.lotes[l].stock += v.cantidad;
            else cout << "  Aviso: no se encontro el lote " << v.numLote << " de " << v.cod << ".\n";
            v.estado = "Cancelada";
        }
    }
    cout << "  Venta #" << idVenta << " cancelada. El inventario fue actualizado.\n";
}

// Lista las ventas (presupuestos de venta) con numero, fecha, vendedor, total y estado.
// Al escribir el numero de una venta se ve que productos se vendieron y se puede cancelar.
void verVentas(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("PRESUPUESTOS DE VENTA - " + u.nomFarmacia);
        int ids[MAX_VENTAS];
        string fechas[MAX_VENTAS];
        string vend[MAX_VENTAS];
        float tot[MAX_VENTAS];
        string est[MAX_VENTAS];
        int n = agruparVentas(s, u.nomFarmacia, ids, fechas, vend, tot, est);
        if (n == 0) {
            cout << "  Aun no hay ventas registradas.\n";
            pausar();
            return;
        }
        float suma = 0;
        cout << left << "  " << setw(6) << "No." << setw(13) << "Fecha" << setw(20) << "Vendedor"
             << setw(12) << "Total" << "Estado\n";
        cout << "  " << string(60, '-') << "\n";
        for (int k = 0; k < n; k++) {
            cout << left << "  " << setw(6) << ids[k] << setw(13) << fechas[k] << setw(20)
                 << cortar(vend[k], 19) << setw(12) << dinero(tot[k]) << est[k] << "\n";
            if (est[k] != "Cancelada") suma += tot[k];
        }
        cout << "\n  Total vendido (sin canceladas): " << dinero(suma) << "\n";
        cout << "\n  Escriba el numero de la venta para ver el detalle (0 = volver)\n";
        op = leerEntero("  Venta: ", 0, 999999);
        if (op != 0) {
            int pos = -1;
            for (int k = 0; k < n; k++)
                if (ids[k] == op) pos = k;
            if (pos == -1) {
                cout << "  Esa venta no existe.\n";
            } else {
                titulo("VENTA #" + aTexto(op));
                cout << "  Fecha:    " << fechas[pos] << "\n  Vendedor: " << vend[pos]
                     << "\n  Estado:   " << est[pos] << "\n\n";
                cout << left << "  " << setw(26) << "Producto" << setw(7) << "Lote" << setw(10)
                     << "Cantidad" << "Total\n";
                cout << "  " << string(54, '-') << "\n";
                for (int i = 0; i < s.totalVentas; i++) {
                    const Venta &v = s.ventas[i];
                    if (v.farmacia == u.nomFarmacia && v.idVenta == op)
                        cout << left << "  " << setw(26) << cortar(nombreProducto(s, v.cod), 25)
                             << setw(7) << v.numLote << setw(10) << v.cantidad << dinero(v.total) << "\n";
                }
                cout << "\n  TOTAL: " << dinero(tot[pos]) << "\n";
                if (est[pos] != "Cancelada") {
                    cout << "\n  1. Cancelar esta venta\n  2. Volver\n";
                    if (leerEntero("  Opcion: ", 1, 2) == 1) cancelarVenta(s, u, op);
                }
            }
            guardarTodo(s);
            pausar();
        }
    } while (op != 0);
}

// Reporte: que vendio cada persona y cuanto. Por cada vendedor muestra cuantas ventas
// hizo, el total y el detalle por producto (unidades y monto).
void reporteVentasPorVendedor(const Sistema &s, const Usuario &u) {
    titulo("VENTAS POR VENDEDOR - " + u.nomFarmacia);
    int ids[MAX_VENTAS];
    string fechas[MAX_VENTAS];
    string vend[MAX_VENTAS];
    float tot[MAX_VENTAS];
    string est[MAX_VENTAS];
    int n = agruparVentas(s, u.nomFarmacia, ids, fechas, vend, tot, est);
    if (n == 0) {
        cout << "  Aun no hay ventas registradas.\n";
        return;
    }
    // lista de vendedores distintos (sin contar ventas canceladas)
    string nombres[MAX_USUARIOS];
    int nV = 0;
    for (int k = 0; k < n; k++) {
        if (est[k] == "Cancelada") continue;   // las ventas canceladas no cuentan
        bool esta = false;
        for (int j = 0; j < nV; j++)
            if (nombres[j] == vend[k]) esta = true;
        if (!esta && nV < MAX_USUARIOS) {
            nombres[nV] = vend[k];
            nV++;
        }
    }
    if (nV == 0) {
        cout << "  Aun no hay ventas vigentes.\n";
        return;
    }
    float granTotal = 0;
    for (int j = 0; j < nV; j++) {
        // cuantas ventas hizo y cuanto sumo
        int cantVentas = 0;
        float totalVend = 0;
        for (int k = 0; k < n; k++)
            if (vend[k] == nombres[j] && est[k] != "Cancelada") {
                cantVentas++;
                totalVend += tot[k];
            }
        // unidades y monto por producto de este vendedor
        int unidades[MAX_PRODUCTOS];
        float monto[MAX_PRODUCTOS];
        for (int i = 0; i < MAX_PRODUCTOS; i++) {
            unidades[i] = 0;
            monto[i] = 0;
        }
        for (int i = 0; i < s.totalVentas; i++) {
            const Venta &v = s.ventas[i];
            if (v.farmacia == u.nomFarmacia && v.vendedor == nombres[j] && v.estado != "Cancelada") {
                int p = buscarProducto(s, v.cod);
                if (p != -1) {
                    unidades[p] += v.cantidad;
                    monto[p] += v.total;
                }
            }
        }
        cout << "\n  Vendedor: " << nombres[j] << "  |  " << cantVentas << " venta(s)  |  Total: "
             << dinero(totalVend) << "\n";
        cout << left << "    " << setw(26) << "Producto" << setw(10) << "Unidades" << "Monto\n";
        cout << "    " << string(46, '-') << "\n";
        for (int p = 0; p < s.totalProductos; p++)
            if (unidades[p] > 0)
                cout << left << "    " << setw(26) << cortar(s.productos[p].nombre, 25)
                     << setw(10) << unidades[p] << dinero(monto[p]) << "\n";
        granTotal += totalVend;
    }
    cout << "\n  Total general de la farmacia: " << dinero(granTotal) << "\n";
}

// Submenu VENTAS
void menuVentas(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("VENTAS");
        cout << "  1. Registrar venta\n  2. Ver ventas (presupuestos de venta)\n"
                "  3. Ventas por vendedor\n  4. Volver\n";
        op = leerEntero("  Opcion: ", 1, 4);
        if (op == 1) registrarVenta(s, u);
        else if (op == 2) verVentas(s, u);
        else if (op == 3) reporteVentasPorVendedor(s, u);
        guardarTodo(s);
        if (op != 4 && op != 2) pausar();   // verVentas ya tiene sus propias pausas
    } while (op != 4);
}

// Reporte: ventas por producto, del mas vendido al menos vendido (burbuja)
void reporteResumenVentas(const Sistema &s, const Usuario &u) {
    titulo("REPORTE: RESUMEN DE VENTAS - " + u.nomFarmacia);
    int unidades[MAX_PRODUCTOS];
    float monto[MAX_PRODUCTOS];
    int idx[MAX_PRODUCTOS];
    for (int i = 0; i < MAX_PRODUCTOS; i++) {
        unidades[i] = 0;
        monto[i] = 0;
        idx[i] = i;
    }
    float totalGeneral = 0;
    int unidadesGeneral = 0;
    // acumular unidades y dinero por producto
    for (int i = 0; i < s.totalVentas; i++) {
        if (s.ventas[i].farmacia == u.nomFarmacia && s.ventas[i].estado != "Cancelada") {
            int p = buscarProducto(s, s.ventas[i].cod);
            if (p != -1) {
                unidades[p] += s.ventas[i].cantidad;
                monto[p] += s.ventas[i].total;
            }
            totalGeneral += s.ventas[i].total;
            unidadesGeneral += s.ventas[i].cantidad;
        }
    }
    if (unidadesGeneral == 0) {
        cout << "  Aun no hay ventas registradas.\n";
        return;
    }
    // ordenar de mas vendido a menos vendido (burbuja sobre las posiciones)
    for (int i = 0; i < s.totalProductos - 1; i++)
        for (int j = 0; j < s.totalProductos - 1 - i; j++)
            if (unidades[idx[j]] < unidades[idx[j + 1]]) {
                int t = idx[j]; idx[j] = idx[j + 1]; idx[j + 1] = t;
            }
    cout << left << "  " << setw(26) << "Producto" << setw(12) << "Unidades" << "Monto\n";
    cout << "  " << string(50, '-') << "\n";
    for (int k = 0; k < s.totalProductos; k++) {
        int p = idx[k];
        if (unidades[p] > 0)
            cout << left << "  " << setw(26) << cortar(s.productos[p].nombre, 25) << setw(12)
                 << unidades[p] << dinero(monto[p]) << "\n";
    }
    cout << "\n  Unidades vendidas: " << unidadesGeneral << "\n  Total vendido: " << dinero(totalGeneral) << "\n";
}

// Submenu REPORTES de la farmacia
void menuReportesFarmacia(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("REPORTES (FARMACIA)");
        cout << "  1. Lotes vencidos / por vencer\n  2. Productos con stock bajo\n"
                "  3. Resumen de ventas\n  4. Ventas por vendedor\n  5. Volver\n";
        op = leerEntero("  Opcion: ", 1, 5);
        if (op == 1) reporteVencimientos(s, u.nomFarmacia);
        else if (op == 2) reporteStockBajo(s, u.nomFarmacia);
        else if (op == 3) reporteResumenVentas(s, u);
        else if (op == 4) reporteVentasPorVendedor(s, u);
        if (op != 5) pausar();
    } while (op != 5);
}

// Submenu USUARIOS de la farmacia. SOLO el encargado entra aqui: crea los usuarios de su
// personal. Cada persona entra con su propio usuario y contrasena, y asi queda
// registrado quien hizo cada venta.
void menuUsuariosFarmacia(Sistema &s, const Usuario &u) {
    if (u.tipo != 3) {
        cout << "  Solo el encargado puede administrar usuarios.\n";
        return;
    }
    int op;
    do {
        titulo("USUARIOS - " + u.nomFarmacia);
        cout << "  1. Crear usuario para el personal\n  2. Ver usuarios de la farmacia\n  3. Volver\n";
        op = leerEntero("  Opcion: ", 1, 3);
        if (op == 1) {
            titulo("CREAR USUARIO");
            crearUsuarioNuevo(s, u.nomFarmacia, 2);
        } else if (op == 2) {
            titulo("USUARIOS DE " + u.nomFarmacia);
            cout << left << "  " << setw(18) << "Usuario" << "Rol\n  " << string(32, '-') << "\n";
            for (int i = 0; i < s.totalUsuarios; i++)
                if (s.usuarios[i].tipo != 1 && s.usuarios[i].nomFarmacia == u.nomFarmacia)
                    cout << left << "  " << setw(18) << s.usuarios[i].nombre
                         << nombreTipo(s.usuarios[i].tipo) << "\n";
        }
        guardarTodo(s);
        if (op != 3) pausar();
    } while (op != 3);
}

// Submenu PRODUCTOS de la farmacia (solo consulta: el stock llega por los envios)
void menuInventarioFarmacia(Sistema &s, const Usuario &u) {
    int op;
    do {
        titulo("PRODUCTOS - " + u.nomFarmacia);
        cout << "  1. Ver inventario\n  2. Buscar producto\n  3. Volver\n";
        op = leerEntero("  Opcion: ", 1, 3);
        if (op == 1) mostrarInventario(s, u.nomFarmacia);
        else if (op == 2) buscarProductos(s, u.nomFarmacia);
        if (op != 3) pausar();
    } while (op != 3);
}

// MENU PRINCIPAL DE LA FARMACIA: productos, pedido a casa medica, ventas y reportes.
// Solo el ENCARGADO ve ademas la opcion de usuarios del personal.
void menuFarmacia(Sistema &s, const Usuario &u) {
    bool esEncargado = (u.tipo == 3);
    int salir = esEncargado ? 6 : 5;   // numero de la opcion "Cerrar sesion"
    int op;
    do {
        titulo(u.nomFarmacia + (esEncargado ? " - Encargado: " : " - Usuario: ") + u.nombre);
        cout << "  1. Productos\n  2. Pedido a casa medica\n  3. Ventas\n  4. Reportes\n";
        if (esEncargado) cout << "  5. Usuarios del personal\n  6. Cerrar sesion\n";
        else cout << "  5. Cerrar sesion\n";
        op = leerEntero("  Opcion: ", 1, salir);
        if (op == 1) menuInventarioFarmacia(s, u);
        else if (op == 2) menuPedidosFarmacia(s, u);
        else if (op == 3) menuVentas(s, u);
        else if (op == 4) menuReportesFarmacia(s, u);
        else if (op == 5 && esEncargado) menuUsuariosFarmacia(s, u);
        else cout << "  Sesion cerrada.\n";
        guardarTodo(s);
    } while (op != salir);
}


// ====================================================================
//  15. LOGIN Y FUNCION PRINCIPAL
// ====================================================================

// Pide usuario y contrasena (3 intentos). Devuelve la posicion del usuario o -1.
int iniciarSesion(const Sistema &s) {
    titulo("INICIO DE SESION");
    for (int intento = 1; intento <= 3; intento++) {
        string nombre = recortar(leerLinea("  Usuario: "));
        string clave = recortar(leerLinea("  Contrasena: "));
        int u = buscarUsuario(s, nombre);
        if (u != -1 && s.usuarios[u].clave == clave) {
            cout << "  Bienvenido, " << nombre << ".\n";
            return u;
        }
        cout << "  Usuario o contrasena incorrectos (intento " << intento << " de 3).\n";
    }
    cout << "  Demasiados intentos fallidos.\n";
    return -1;
}

// main() solo coordina: carga los datos, muestra el login y manda a cada
// usuario a su menu (casa medica o farmacia) segun su tipo.
int main() {
    static Sistema sis;   // static porque es muy grande para la pila
    cargarTodo(sis);
    cargarDatosIniciales(sis);   // agrega los productos iniciales que falten
    guardarTodo(sis);
    titulo("SISTEMA DE INVENTARIO - CASA MEDICA Y FARMACIAS");
    int op;
    do {
        cout << "\n  1. Iniciar sesion\n  2. Salir\n";
        op = leerEntero("  Opcion: ", 1, 2);
        if (op == 1) {
            int u = iniciarSesion(sis);
            if (u != -1) {
                Usuario actual = sis.usuarios[u];
                if (actual.tipo == 1) menuCasaMedica(sis, actual);
                else menuFarmacia(sis, actual);
            }
        }
    } while (op != 2);
    guardarTodo(sis);
    cout << "  Hasta luego.\n";
    pausar();   // evita que la ventana se cierre sola en Dev-C++
    return 0;
}
