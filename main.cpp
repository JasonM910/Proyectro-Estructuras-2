#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

/**
 * @brief Representa un registro demografico de estudiante almacenado en el archivo binario de estudiantes.
 */
struct Estudiante {
    string carne;
    string genero;
    string residencia;
    int edad = 0;
    string colegioProcedencia;
    string tipoColegio;
    bool trabaja = false;
    string estadoCivil;
};

/**
 * @brief Representa un registro de nota dentro del archivo binario de historial academico.
 */
struct RegistroHistorial {
    string carneEstudiante;
    int semestre = 0;
    string materia;
    double nota = 0.0;
};

/**
 * @brief Agrupa a un estudiante con las estadisticas derivadas de sus registros de notas.
 */
struct PerfilEstudiante {
    Estudiante estudiante;
    vector<RegistroHistorial> historial;
    optional<double> promedio;
    optional<double> tasaAprobacion;
};

namespace fs = filesystem;

namespace {

constexpr const char *kArchivoEstudiantes = "estudiantes.bin";
constexpr const char *kArchivoHistorial = "historial.bin";

/**
 * @brief Escribe una cadena con prefijo de longitud en un flujo binario.
 * @param out Flujo de salida abierto en modo binario.
 * @param valor Cadena que se escribira.
 */
void escribirCadena(ofstream &out, const string &valor) {
    const auto longitud = static_cast<uint32_t>(valor.size());
    out.write(reinterpret_cast<const char *>(&longitud), sizeof(longitud));
    out.write(valor.data(), static_cast<streamsize>(longitud));
}

/**
 * @brief Lee una cadena con prefijo de longitud desde un flujo binario.
 * @param in Flujo de entrada abierto en modo binario.
 * @param valor Parametro de salida que recibe la cadena decodificada.
 * @return true si la cadena se leyo con exito; false en fin de archivo o error.
 */
bool leerCadena(ifstream &in, string &valor) {
    uint32_t longitud = 0;
    if (!in.read(reinterpret_cast<char *>(&longitud), sizeof(longitud))) {
        return false;
    }
    if (longitud > 10'000) {
        throw runtime_error("Longitud de cadena invalida encontrada en el archivo binario.");
    }
    string buffer(longitud, '\0');
    if (!buffer.empty()) {
        if (!in.read(buffer.data(), static_cast<streamsize>(longitud))) {
            return false;
        }
    }
    valor = move(buffer);
    return true;
}

/**
 * @brief Escribe un valor booleano en un flujo binario como un unico byte.
 * @param out Flujo de salida abierto en modo binario.
 * @param valor Indicador booleano a persistir.
 */
void escribirBooleano(ofstream &out, bool valor) {
    uint8_t flag = valor ? 1U : 0U;
    out.write(reinterpret_cast<const char *>(&flag), sizeof(flag));
}

/**
 * @brief Lee un valor booleano de un flujo binario codificado como un unico byte.
 * @param in Flujo de entrada abierto en modo binario.
 * @param valor Parametro de salida que recibe el indicador decodificado.
 * @return true si el indicador se leyo con exito; false en caso contrario.
 */
bool leerBooleano(ifstream &in, bool &valor) {
    uint8_t flag = 0;
    if (!in.read(reinterpret_cast<char *>(&flag), sizeof(flag))) {
        return false;
    }
    valor = flag != 0;
    return true;
}

/**
 * @brief Lee una estructura Estudiante desde el flujo.
 * @param in Flujo de entrada en modo binario.
 * @param estudiante Parametro de salida que recibe el estudiante decodificado.
 * @return true si el registro se leyo; false en fin de archivo o error.
 */
bool leerEstudiante(ifstream &in, Estudiante &estudiante) {
    Estudiante temporal;
    if (!leerCadena(in, temporal.carne)) {
        return false;
    }
    if (!leerCadena(in, temporal.genero)) {
        return false;
    }
    if (!leerCadena(in, temporal.residencia)) {
        return false;
    }
    if (!in.read(reinterpret_cast<char *>(&temporal.edad), sizeof(temporal.edad))) {
        return false;
    }
    if (!leerCadena(in, temporal.colegioProcedencia)) {
        return false;
    }
    if (!leerCadena(in, temporal.tipoColegio)) {
        return false;
    }
    if (!leerBooleano(in, temporal.trabaja)) {
        return false;
    }
    if (!leerCadena(in, temporal.estadoCivil)) {
        return false;
    }
    estudiante = move(temporal);
    return true;
}

/**
 * @brief Escribe la estructura Estudiante en el flujo.
 * @param out Flujo de salida en modo binario.
 * @param estudiante Registro a persistir.
 */
void escribirEstudiante(ofstream &out, const Estudiante &estudiante) {
    escribirCadena(out, estudiante.carne);
    escribirCadena(out, estudiante.genero);
    escribirCadena(out, estudiante.residencia);
    out.write(reinterpret_cast<const char *>(&estudiante.edad), sizeof(estudiante.edad));
    escribirCadena(out, estudiante.colegioProcedencia);
    escribirCadena(out, estudiante.tipoColegio);
    escribirBooleano(out, estudiante.trabaja);
    escribirCadena(out, estudiante.estadoCivil);
}

/**
 * @brief Lee una estructura RegistroHistorial desde el flujo.
 * @param in Flujo de entrada en modo binario.
 * @param registro Parametro de salida que recibe el registro decodificado.
 * @return true si el registro se leyo; false en caso contrario.
 */
bool leerRegistroHistorial(ifstream &in, RegistroHistorial &registro) {
    RegistroHistorial temporal;
    if (!leerCadena(in, temporal.carneEstudiante)) {
        return false;
    }
    if (!in.read(reinterpret_cast<char *>(&temporal.semestre), sizeof(temporal.semestre))) {
        return false;
    }
    if (!leerCadena(in, temporal.materia)) {
        return false;
    }
    if (!in.read(reinterpret_cast<char *>(&temporal.nota), sizeof(temporal.nota))) {
        return false;
    }
    registro = move(temporal);
    return true;
}

/**
 * @brief Escribe la estructura RegistroHistorial en el flujo.
 * @param out Flujo de salida en modo binario.
 * @param registro Registro a persistir.
 */
void escribirRegistroHistorial(ofstream &out, const RegistroHistorial &registro) {
    escribirCadena(out, registro.carneEstudiante);
    out.write(reinterpret_cast<const char *>(&registro.semestre), sizeof(registro.semestre));
    escribirCadena(out, registro.materia);
    out.write(reinterpret_cast<const char *>(&registro.nota), sizeof(registro.nota));
}

/**
 * @brief Elimina espacios en blanco en ambos extremos de una cadena.
 * @param texto Cadena original.
 * @return Cadena sin espacios iniciales ni finales.
 */
string recortar(const string &texto) {
    auto inicio = texto.find_first_not_of(" \t\n\r\f\v");
    auto fin = texto.find_last_not_of(" \t\n\r\f\v");
    if (inicio == string::npos) {
        return {};
    }
    return texto.substr(inicio, fin - inicio + 1);
}

/**
 * @brief Convierte una cadena a mayusculas.
 * @param texto Cadena original.
 * @return Copia en mayusculas.
 */
string aMayusculas(const string &texto) {
    string resultado = texto;
    transform(resultado.begin(), resultado.end(), resultado.begin(),
                   [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
    return resultado;
}

/**
 * @brief Devuelve el tamano del archivo en bytes o cero si no existe.
 * @param ruta Ruta en el sistema de archivos.
 * @return Tamano en bytes o cero si hay fallo.
 */
uintmax_t tamanoArchivoSeguro(const string &ruta) {
    if (!fs::exists(ruta)) {
        return 0U;
    }
    error_code ec;
    const auto tamano = fs::file_size(ruta, ec);
    if (ec) {
        return 0U;
    }
    return tamano;
}

} // namespace

/**
 * @brief Proporciona persistencia binaria para registros de estudiantes.
 */
class RepositorioEstudiantes {
public:
    explicit RepositorioEstudiantes(string ruta) : ruta_(move(ruta)) {}

    /**
     * @brief Carga todos los estudiantes desde el disco.
     * @return Vector con cada estudiante persistido.
     */
    vector<Estudiante> cargarTodos() const {
        vector<Estudiante> estudiantes;
        ifstream in(ruta_, ios::binary);
        if (!in.is_open()) {
            return estudiantes;
        }
        Estudiante estudiante;
        while (leerEstudiante(in, estudiante)) {
            estudiantes.push_back(estudiante);
        }
        return estudiantes;
    }

    /**
     * @brief Anade un nuevo estudiante al disco.
     * @param estudiante Registro a persistir.
     * @throws runtime_error cuando el identificador ya existe o no se puede abrir el archivo.
     */
    void agregar(const Estudiante &estudiante) const {
        if (existe(estudiante.carne)) {
            throw runtime_error("El carne ingresado ya esta registrado.");
        }
        ofstream out(ruta_, ios::binary | ios::app);
        if (!out.is_open()) {
            throw runtime_error("No se pudo abrir el archivo de estudiantes para escritura.");
        }
        escribirEstudiante(out, estudiante);
    }

    /**
     * @brief Verifica si existe un estudiante con el identificador indicado.
     * @param carne Carne que se desea buscar.
     * @return true si el estudiante existe; false en caso contrario.
     */
    bool existe(const string &carne) const {
        ifstream in(ruta_, ios::binary);
        if (!in.is_open()) {
            return false;
        }
        Estudiante estudiante;
        while (leerEstudiante(in, estudiante)) {
            if (estudiante.carne == carne) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Garantiza que el archivo del repositorio exista en disco.
     */
    void asegurarArchivo() const {
        if (!fs::exists(ruta_)) {
            ofstream out(ruta_, ios::binary);
        }
    }

    /**
     * @brief Devuelve la ruta del archivo del repositorio.
     * @return Referencia constante a la cadena de ruta.
     */
    [[nodiscard]] const string &ruta() const {
        return ruta_;
    }

private:
    string ruta_;
};

/**
 * @brief Proporciona persistencia binaria para los registros de historial academico.
 */
class RepositorioHistorial {
public:
    explicit RepositorioHistorial(string ruta) : ruta_(move(ruta)) {}

    /**
     * @brief Carga todos los registros de historial desde el disco.
     * @return Vector con cada registro persistido.
     */
    vector<RegistroHistorial> cargarTodos() const {
        vector<RegistroHistorial> registros;
        ifstream in(ruta_, ios::binary);
        if (!in.is_open()) {
            return registros;
        }
        RegistroHistorial registro;
        while (leerRegistroHistorial(in, registro)) {
            registros.push_back(registro);
        }
        return registros;
    }

    /**
     * @brief Anade un registro de historial al disco.
     * @param registro Registro a persistir.
     * @throws runtime_error cuando no se puede abrir el archivo.
     */
    void agregar(const RegistroHistorial &registro) const {
        ofstream out(ruta_, ios::binary | ios::app);
        if (!out.is_open()) {
            throw runtime_error("No se pudo abrir el archivo de historial para escritura.");
        }
        escribirRegistroHistorial(out, registro);
    }

    /**
     * @brief Garantiza que el archivo del repositorio exista en disco.
     */
    void asegurarArchivo() const {
        if (!fs::exists(ruta_)) {
            ofstream out(ruta_, ios::binary);
        }
    }

    /**
     * @brief Devuelve la ruta del archivo del repositorio.
     * @return Referencia constante a la cadena de ruta.
     */
    [[nodiscard]] const string &ruta() const {
        return ruta_;
    }

private:
    string ruta_;
};

/**
 * @brief Carga perfiles de estudiantes con estadisticas desde los repositorios.
 * @param repositorioEstudiantes Repositorio que suministra los registros de estudiantes.
 * @param repositorioHistorial Repositorio que suministra los registros de historial.
 * @return Vector de perfiles de estudiantes con promedios y tasas de aprobacion calculadas.
 */
vector<PerfilEstudiante> cargarPerfiles(const RepositorioEstudiantes &repositorioEstudiantes,
                                        const RepositorioHistorial &repositorioHistorial) {
    const auto estudiantes = repositorioEstudiantes.cargarTodos();
    const auto registrosHistorial = repositorioHistorial.cargarTodos();

    unordered_map<string, vector<RegistroHistorial>> registrosPorEstudiante;
    registrosPorEstudiante.reserve(registrosHistorial.size());
    for (const auto &registro : registrosHistorial) {
        registrosPorEstudiante[registro.carneEstudiante].push_back(registro);
    }

    vector<PerfilEstudiante> perfiles;
    perfiles.reserve(estudiantes.size());
    for (const auto &estudiante : estudiantes) {
        PerfilEstudiante perfil;
        perfil.estudiante = estudiante;
        if (auto it = registrosPorEstudiante.find(estudiante.carne); it != registrosPorEstudiante.end()) {
            perfil.historial = move(it->second);
            if (!perfil.historial.empty()) {
                double suma = 0.0;
                size_t aprobadas = 0;
                for (const auto &entrada : perfil.historial) {
                    suma += entrada.nota;
                    if (entrada.nota >= 70.0) {
                        ++aprobadas;
                    }
                }
                perfil.promedio = suma / static_cast<double>(perfil.historial.size());
                perfil.tasaAprobacion =
                    static_cast<double>(aprobadas) / static_cast<double>(perfil.historial.size());
            }
        }
        perfiles.push_back(move(perfil));
    }
    return perfiles;
}

/**
 * @brief Enumeracion de las variables de clasificacion disponibles.
 */
enum class VariableClasificacion {
    Genero = 0,
    Residencia,
    TipoColegio,
    RangoEdad,
    RangoPromedio,
    RangoAprobacion,
    Trabaja,
    EstadoCivil,
    ColegioProcedencia
};

/**
 * @brief Devuelve el nombre legible de una variable de clasificacion.
 * @param variable Variable que se desea describir.
 * @return Nombre descriptivo para mostrar.
 */
string variableComoCadena(VariableClasificacion variable) {
    switch (variable) {
        case VariableClasificacion::Genero:
            return "Genero";
        case VariableClasificacion::Residencia:
            return "Lugar de residencia";
        case VariableClasificacion::TipoColegio:
            return "Tipo de colegio";
        case VariableClasificacion::RangoEdad:
            return "Rango de edad";
        case VariableClasificacion::RangoPromedio:
            return "Promedio de notas";
        case VariableClasificacion::RangoAprobacion:
            return "Porcentaje de aprobacion";
        case VariableClasificacion::Trabaja:
            return "Trabaja";
        case VariableClasificacion::EstadoCivil:
            return "Estado civil";
        case VariableClasificacion::ColegioProcedencia:
            return "Colegio de procedencia";
        default:
            return "Variable desconocida";
    }
}

/**
 * @brief Devuelve el rango etiquetado para un valor de edad.
 * @param age Edad en anos.
 * @return Descripcion del rango.
 */
string rangoEdad(int edad) {
    if (edad <= 0) {
        return "Edad desconocida";
    }
    if (edad < 18) {
        return "Menor a 18";
    }
    if (edad <= 30) {
        return "18-30";
    }
    if (edad <= 64) {
        return "31-64";
    }
    return "65+";
}

/**
 * @brief Devuelve una etiqueta que describe el rango del promedio de notas.
 * @param promedio Promedio de notas expresado de 0 a 100.
 * @return Etiqueta del rango o "Sin historial" si no hay datos.
 */
string rangoPromedio(const optional<double> &promedio) {
    if (!promedio.has_value()) {
        return "Sin historial";
    }
    const double valor = clamp(promedio.value(), 0.0, 100.0);
    if (valor < 60.0) {
        return "0-59";
    }
    if (valor < 80.0) {
        return "60-79";
    }
    return "80-100";
}

/**
 * @brief Devuelve una etiqueta que describe el rango del porcentaje de aprobacion.
 * @param tasaAprobacion Proporcion de aprobacion como fraccion entre 0 y 1.
 * @return Etiqueta del rango o "Sin historial" si no hay datos.
 */
string rangoAprobacion(const optional<double> &tasaAprobacion) {
    if (!tasaAprobacion.has_value()) {
        return "Sin historial";
    }
    const double porcentaje = clamp(tasaAprobacion.value() * 100.0, 0.0, 100.0);
    if (porcentaje <= 50.0) {
        return "0-50 %";
    }
    if (porcentaje <= 75.0) {
        return "51-75 %";
    }
    return "76-100 %";
}

/**
 * @brief Calcula la etiqueta de clasificacion de una variable usando el perfil del estudiante.
 * @param variable Variable objetivo.
 * @param profile Perfil del estudiante que contiene los datos.
 * @return Etiqueta de clasificacion.
 */
string valorClasificacion(VariableClasificacion variable, const PerfilEstudiante &perfil) {
    switch (variable) {
        case VariableClasificacion::Genero:
            return perfil.estudiante.genero.empty() ? "Sin registro" : perfil.estudiante.genero;
        case VariableClasificacion::Residencia:
            return perfil.estudiante.residencia.empty() ? "Sin registro" : perfil.estudiante.residencia;
        case VariableClasificacion::TipoColegio:
            return perfil.estudiante.tipoColegio.empty() ? "Sin registro" : perfil.estudiante.tipoColegio;
        case VariableClasificacion::RangoEdad:
            return rangoEdad(perfil.estudiante.edad);
        case VariableClasificacion::RangoPromedio:
            return rangoPromedio(perfil.promedio);
        case VariableClasificacion::RangoAprobacion:
            return rangoAprobacion(perfil.tasaAprobacion);
        case VariableClasificacion::Trabaja:
            return perfil.estudiante.trabaja ? "Si" : "No";
        case VariableClasificacion::EstadoCivil:
            return perfil.estudiante.estadoCivil.empty() ? "Sin registro"
                                                         : perfil.estudiante.estadoCivil;
        case VariableClasificacion::ColegioProcedencia:
            return perfil.estudiante.colegioProcedencia.empty() ? "Sin registro"
                                                                : perfil.estudiante.colegioProcedencia;
        default:
            return "Desconocido";
    }
}

/**
 * @brief Nodo del arbol de clasificacion que almacena indices de estudiantes y relaciones jerarquicas.
 */
struct NodoArbolClasificacion {
    string etiqueta;
    optional<VariableClasificacion> variable;
    vector<size_t> indicesEstudiantes;
    vector<unique_ptr<NodoArbolClasificacion>> hijos;
    NodoArbolClasificacion *padre = nullptr;
    size_t nivel = 0;
};

/**
 * @brief Construye el arbol de clasificacion de forma recursiva.
 * @param nodo Nodo cuyos hijos seran poblados.
 * @param profiles Perfiles de estudiantes utilizados para agrupar.
 * @param orden Secuencia de variables de clasificacion.
 */
void construirArbolRecursivo(NodoArbolClasificacion &nodo, const vector<PerfilEstudiante> &perfiles,
                             const vector<VariableClasificacion> &orden) {
    if (nodo.nivel >= orden.size()) {
        return;
    }

    const auto variable = orden[nodo.nivel];
    map<string, vector<size_t>> grupos;
    for (const auto indice : nodo.indicesEstudiantes) {
        const auto &perfil = perfiles.at(indice);
        const auto etiqueta = valorClasificacion(variable, perfil);
        grupos[etiqueta].push_back(indice);
    }

    for (auto &[etiqueta, indices] : grupos) {
        auto hijo = make_unique<NodoArbolClasificacion>();
        hijo->etiqueta = etiqueta;
        hijo->variable = variable;
        hijo->indicesEstudiantes = move(indices);
        hijo->padre = &nodo;
        hijo->nivel = nodo.nivel + 1;
        construirArbolRecursivo(*hijo, perfiles, orden);
        nodo.hijos.push_back(move(hijo));
    }
}

/**
 * @brief Construye el arbol de clasificacion usando el orden de variables indicado.
 * @param perfiles Perfiles de estudiantes que participan en el arbol.
 * @param orden Secuencia de variables de clasificacion (niveles).
 * @return Puntero al nodo raiz.
 */
unique_ptr<NodoArbolClasificacion>
construirArbolClasificacion(const vector<PerfilEstudiante> &perfiles,
                            const vector<VariableClasificacion> &orden) {
    auto raiz = make_unique<NodoArbolClasificacion>();
    raiz->etiqueta = "Poblacion total";
    raiz->nivel = 0;
    raiz->indicesEstudiantes.reserve(perfiles.size());
    for (size_t indice = 0; indice < perfiles.size(); ++indice) {
        raiz->indicesEstudiantes.push_back(indice);
    }
    construirArbolRecursivo(*raiz, perfiles, orden);
    return raiz;
}

/**
 * @brief Recolecta todos los nodos hoja del arbol de clasificacion.
 * @param nodo Nodo examinado durante el recorrido.
 * @param hojas Coleccion de salida donde se almacenan los nodos hoja.
 */
void recolectarHojas(const NodoArbolClasificacion &nodo,
                     vector<const NodoArbolClasificacion *> &hojas) {
    if (nodo.hijos.empty()) {
        hojas.push_back(&nodo);
        return;
    }
    for (const auto &hijo : nodo.hijos) {
        recolectarHojas(*hijo, hojas);
    }
}

/**
 * @brief Devuelve la ruta jerarquica desde la raiz hasta el nodo indicado.
 * @param nodo Nodo objetivo.
 * @return Vector con los nodos desde la raiz hasta el nodo indicado.
 */
vector<const NodoArbolClasificacion *> rutaHastaRaiz(const NodoArbolClasificacion *nodo) {
    vector<const NodoArbolClasificacion *> ruta;
    auto actual = nodo;
    while (actual != nullptr) {
        ruta.push_back(actual);
        actual = actual->padre;
    }
    reverse(ruta.begin(), ruta.end());
    return ruta;
}

/**
 * @brief Imprime el arbol de clasificacion por niveles.
 * @param root Nodo raiz del arbol.
 */
void imprimirArbolPorNiveles(const NodoArbolClasificacion &raiz) {
    queue<const NodoArbolClasificacion *> cola;
    cola.push(&raiz);
    size_t nivelActual = 0;

    while (!cola.empty()) {
        const auto cantidadNivel = cola.size();
        cout << "Nivel " << nivelActual << ":\n";
        for (size_t i = 0; i < cantidadNivel; ++i) {
            const auto nodo = cola.front();
            cola.pop();

            string descriptor;
            if (!nodo->variable.has_value()) {
                descriptor = nodo->etiqueta;
            } else {
                descriptor =
                    variableComoCadena(nodo->variable.value()) + " = " + nodo->etiqueta;
            }
            cout << "  - " << descriptor << " (" << nodo->indicesEstudiantes.size()
                      << " estudiantes)\n";
            for (const auto &hijo : nodo->hijos) {
                cola.push(hijo.get());
            }
        }
        ++nivelActual;
    }
    cout << endl;
}

/**
 * @brief Imprime un perfil de estudiante combinando datos personales e historial academico.
 * @param profile Perfil de estudiante a mostrar.
 */
void imprimirPerfil(const PerfilEstudiante &perfil) {
    const auto &estudiante = perfil.estudiante;
    cout << "Carne: " << estudiante.carne << " | Genero: " << estudiante.genero
              << " | Residencia: " << estudiante.residencia << " | Edad: " << estudiante.edad
              << " | Colegio origen: " << estudiante.colegioProcedencia
              << " | Tipo colegio: " << estudiante.tipoColegio
              << " | Trabaja: " << (estudiante.trabaja ? "Si" : "No")
              << " | Estado civil: " << estudiante.estadoCivil << '\n';

    if (perfil.historial.empty()) {
        cout << "  Historial: Sin registros.\n";
        return;
    }

    cout << "  Historial (" << perfil.historial.size() << " registros):\n";
    for (const auto &entrada : perfil.historial) {
        cout << "    - Semestre " << entrada.semestre << " | Materia: " << entrada.materia
                  << " | Nota: " << fixed << setprecision(2) << entrada.nota << '\n';
    }
    if (perfil.promedio.has_value()) {
        cout << "  Promedio: " << fixed << setprecision(2)
                  << perfil.promedio.value() << '\n';
    }
    if (perfil.tasaAprobacion.has_value()) {
        cout << "  % Aprobacion: " << fixed << setprecision(2)
                  << perfil.tasaAprobacion.value() * 100.0 << "%\n";
    }
}

/**
 * @brief Precarga datos de demostracion cuando los repositorios estan vacios.
 * @param repositorioEstudiantes Repositorio de estudiantes.
 * @param repositorioHistorial Repositorio de historial.
 */
void precargarDatos(const RepositorioEstudiantes &repositorioEstudiantes,
                    const RepositorioHistorial &repositorioHistorial) {
    if (tamanoArchivoSeguro(repositorioEstudiantes.ruta()) > 0 &&
        tamanoArchivoSeguro(repositorioHistorial.ruta()) > 0) {
        return;
    }

    vector<string> generos = {"Masculino", "Femenino"};
    vector<string> residencias = {"San Jose", "Alajuela", "Cartago", "Heredia"};
    vector<string> tiposColegio = {"Publico", "Privado", "Tecnico"};
    vector<string> estadosCiviles = {"Soltero", "Casado"};
    vector<string> colegios = {"Liceo Central", "Colegio Tecnico", "Instituto Moderno"};

    for (int indiceEstudiante = 0; indiceEstudiante < 30; ++indiceEstudiante) {
        Estudiante estudiante;
        ostringstream generadorCarne;
        generadorCarne << "A" << setfill('0') << setw(3) << indiceEstudiante + 1;
        estudiante.carne = generadorCarne.str();
        estudiante.genero = generos[indiceEstudiante % generos.size()];
        estudiante.residencia = residencias[indiceEstudiante % residencias.size()];
        estudiante.edad = 18 + (indiceEstudiante % 15);
        estudiante.colegioProcedencia = colegios[indiceEstudiante % colegios.size()];
        estudiante.tipoColegio = tiposColegio[indiceEstudiante % tiposColegio.size()];
        estudiante.trabaja = (indiceEstudiante % 3 == 0);
        estudiante.estadoCivil = estadosCiviles[indiceEstudiante % estadosCiviles.size()];
        repositorioEstudiantes.agregar(estudiante);

        for (int indiceRegistro = 0; indiceRegistro < 4; ++indiceRegistro) {
            RegistroHistorial registro;
            registro.carneEstudiante = estudiante.carne;
            registro.semestre = 1 + (indiceRegistro % 4);
            ostringstream generadorMateria;
            generadorMateria << "Materia " << (indiceRegistro + 1);
            registro.materia = generadorMateria.str();
            registro.nota = 55.0 + (indiceEstudiante * 3 + indiceRegistro * 5) % 45;
            repositorioHistorial.agregar(registro);
        }
    }
}

/**
 * @brief Aplicacion simple basada en consola que coordina las acciones del menu.
 */
class Aplicacion {
public:
    Aplicacion()
        : repositorioEstudiantes_(kArchivoEstudiantes),
          repositorioHistorial_(kArchivoHistorial),
          perfiles_(cargarPerfiles(repositorioEstudiantes_, repositorioHistorial_)) {
        repositorioEstudiantes_.asegurarArchivo();
        repositorioHistorial_.asegurarArchivo();
        precargarDatos(repositorioEstudiantes_, repositorioHistorial_);
        perfiles_ = cargarPerfiles(repositorioEstudiantes_, repositorioHistorial_);
    }

    /**
     * @brief Inicia el bucle principal de interaccion.
     */
    void ejecutar() {
        bool enEjecucion = true;
        while (enEjecucion) {
            imprimirMenuPrincipal();
            const auto opcion = solicitar("Seleccione una opcion");
            if (opcion == "1") {
                opcionConstruirArbol();
            } else if (opcion == "2") {
                opcionImprimirArbol();
            } else if (opcion == "3") {
                opcionPorcentajesCondicionados();
            } else if (opcion == "4") {
                opcionReporteHojas();
            } else if (opcion == "5") {
                opcionImprimirPerfiles();
            } else if (opcion == "6") {
                opcionAgregarEstudiante();
            } else if (opcion == "7") {
                opcionAgregarNota();
            } else if (opcion == "0") {
                enEjecucion = false;
            } else {
                cout << "Opcion no valida. Intente de nuevo.\n";
            }
        }
        cout << "Hasta pronto.\n";
    }

private:
    RepositorioEstudiantes repositorioEstudiantes_;
    RepositorioHistorial repositorioHistorial_;
    vector<PerfilEstudiante> perfiles_;
    vector<VariableClasificacion> ordenActivo_;
    unique_ptr<NodoArbolClasificacion> arbolActual_;

    /**
     * @brief Imprime las opciones del menu principal.
     */
    static void imprimirMenuPrincipal() {
        cout << "\n=== Clasificacion de Estudiantes ===\n";
        cout << "1. Construir un nuevo arbol de clasificacion\n";
        cout << "2. Imprimir arbol por niveles\n";
        cout << "3. Calcular porcentajes condicionados\n";
        cout << "4. Imprimir totales y porcentajes por hoja\n";
        cout << "5. Listar estudiantes y su historial\n";
        cout << "6. Registrar nuevo estudiante\n";
        cout << "7. Registrar nueva nota en historial\n";
        cout << "0. Salir\n";
    }

    /**
     * @brief Solicita entrada al usuario y recorta la respuesta.
     * @param mensaje Mensaje mostrado antes de esperar la entrada.
     * @return Entrada del usuario sin espacios (posiblemente vacia).
     */
    static string solicitar(const string &mensaje) {
        cout << mensaje << ": ";
        string texto;
        getline(cin, texto);
        return recortar(texto);
    }

    /**
     * @brief Pregunta hasta que el usuario proporcione una respuesta no vacia.
     * @param mensaje Mensaje mostrado al usuario.
     * @return Cadena no vacia.
     */
    static string solicitarNoVacio(const string &mensaje) {
        while (true) {
            const auto valor = solicitar(mensaje);
            if (!valor.empty()) {
                return valor;
            }
            cout << "El valor no puede estar vacio. Intente nuevamente.\n";
        }
    }

    /**
     * @brief Solicita al usuario un entero dentro de un rango.
     * @param mensaje Mensaje mostrado al solicitar datos.
     * @param valorMinimo Valor minimo aceptado.
     * @param valorMaximo Valor maximo aceptado.
     * @return Entero valido dentro del rango.
     */
    static int solicitarEntero(const string &mensaje, int valorMinimo, int valorMaximo) {
        while (true) {
            const auto valor = solicitar(mensaje);
            try {
                const int numero = stoi(valor);
                if (numero < valorMinimo || numero > valorMaximo) {
                    throw out_of_range("Fuera de rango");
                }
                return numero;
            } catch (const exception &) {
                cout << "Valor invalido. Ingrese un numero entre " << valorMinimo << " y "
                          << valorMaximo << ".\n";
            }
        }
    }

    /**
     * @brief Solicita al usuario un valor flotante dentro de un rango.
     * @param mensaje Mensaje mostrado al solicitar datos.
     * @param valorMinimo Valor minimo aceptado.
     * @param valorMaximo Valor maximo aceptado.
     * @return Numero valido dentro del rango.
     */
    static double solicitarDoble(const string &mensaje, double valorMinimo, double valorMaximo) {
        while (true) {
            const auto valor = solicitar(mensaje);
            try {
                const double numero = stod(valor);
                if (numero < valorMinimo || numero > valorMaximo) {
                    throw out_of_range("Fuera de rango");
                }
                return numero;
            } catch (const exception &) {
                cout << "Valor invalido. Ingrese un numero entre " << fixed
                          << setprecision(2) << valorMinimo << " y " << valorMaximo << ".\n";
            }
        }
    }

    /**
     * @brief Recarga los perfiles desde el almacenamiento y reconstruye el arbol activo si es necesario.
     */
    void actualizarPerfiles() {
        perfiles_ = cargarPerfiles(repositorioEstudiantes_, repositorioHistorial_);
        if (!ordenActivo_.empty()) {
            arbolActual_ = construirArbolClasificacion(perfiles_, ordenActivo_);
        }
    }

    /**
     * @brief Construye un nuevo arbol de clasificacion segun las variables seleccionadas por el usuario.
     */
    void opcionConstruirArbol() {
        if (perfiles_.empty()) {
            cout << "No hay estudiantes registrados. Registre estudiantes antes de "
                         "construir el arbol.\n";
            return;
        }

        const auto orden = solicitarOrdenClasificacion();
        if (orden.empty()) {
            cout << "No se seleccionaron variables. Operacion cancelada.\n";
            return;
        }
        ordenActivo_ = orden;
        arbolActual_ = construirArbolClasificacion(perfiles_, ordenActivo_);
        cout << "Arbol construido correctamente con " << ordenActivo_.size()
                  << " niveles de clasificacion.\n";
    }

    /**
     * @brief Muestra el catalogo de variables que pueden usarse para construir el arbol.
     */
    static void imprimirVariablesDisponibles() {
        cout << "Variables disponibles:\n";
        cout << " 1. Genero\n";
        cout << " 2. Lugar de residencia\n";
        cout << " 3. Tipo de colegio\n";
        cout << " 4. Rango de edad\n";
        cout << " 5. Promedio de notas\n";
        cout << " 6. Porcentaje de aprobacion\n";
        cout << " 7. Trabaja\n";
        cout << " 8. Estado civil\n";
        cout << " 9. Colegio de procedencia\n";
    }

    /**
     * @brief Convierte una opcion del menu en una variable de clasificacion.
     * @param option Numero seleccionado por el usuario.
     * @return Variable cuando la opcion es valida; nullopt en caso contrario.
     */
    static optional<VariableClasificacion> variableDesdeOpcion(int option) {
        switch (option) {
            case 1:
                return VariableClasificacion::Genero;
            case 2:
                return VariableClasificacion::Residencia;
            case 3:
                return VariableClasificacion::TipoColegio;
            case 4:
                return VariableClasificacion::RangoEdad;
            case 5:
                return VariableClasificacion::RangoPromedio;
            case 6:
                return VariableClasificacion::RangoAprobacion;
            case 7:
                return VariableClasificacion::Trabaja;
            case 8:
                return VariableClasificacion::EstadoCivil;
            case 9:
                return VariableClasificacion::ColegioProcedencia;
            default:
                return nullopt;
        }
    }

    /**
     * @brief Captura de forma interactiva el orden de variables para construir el arbol.
     * @return Secuencia de variables unicas. Puede estar vacia si el usuario termina antes.
     */
    vector<VariableClasificacion> solicitarOrdenClasificacion() {
        vector<VariableClasificacion> orden;
        vector<bool> variablesUsadas(10, false);
        const int maximoNiveles = 8;
        while (static_cast<int>(orden.size()) < maximoNiveles) {
            imprimirVariablesDisponibles();
            cout << "Seleccione el numero de la variable para el nivel " << (orden.size() + 1)
                      << " (Enter para finalizar): ";
            string entrada;
            getline(cin, entrada);
            entrada = recortar(entrada);
            if (entrada.empty()) {
                break;
            }
            try {
                const int opcion = stoi(entrada);
                if (opcion < 1 || opcion > 9) {
                    throw out_of_range("rango");
                }
                if (variablesUsadas[opcion]) {
                    cout << "La variable ya fue seleccionada. Elija otra.\n";
                    continue;
                }
                if (const auto variable = variableDesdeOpcion(opcion); variable.has_value()) {
                    orden.push_back(variable.value());
                    variablesUsadas[opcion] = true;
                }
            } catch (const exception &) {
                cout << "Entrada invalida. Intente nuevamente.\n";
            }
        }
        return orden;
    }

    /**
     * @brief Imprime el arbol actual agrupado por niveles.
     */
    void opcionImprimirArbol() {
        if (!arbolListo()) {
            return;
        }
        imprimirArbolPorNiveles(*arbolActual_);
    }

    /**
     * @brief Valida que se haya construido un arbol de clasificacion.
     * @return true cuando el arbol existe; false en caso contrario (se imprime una guia).
     */
    bool arbolListo() {
        if (!arbolActual_) {
            cout << "Aun no se ha construido un arbol. Seleccione la opcion 1 primero.\n";
            return false;
        }
        return true;
    }

    /**
     * @brief Permite al usuario navegar el arbol y obtener porcentajes condicionados.
     */
    void opcionPorcentajesCondicionados() {
        if (!arbolListo()) {
            return;
        }
        auto *nodo = arbolActual_.get();
        NodoArbolClasificacion *padre = nullptr;
        for (size_t nivel = 0; nivel < ordenActivo_.size(); ++nivel) {
            if (nodo->hijos.empty()) {
                break;
            }
            const auto variable = ordenActivo_[nivel];
            cout << "\n" << variableComoCadena(variable) << " disponibles:\n";
            for (size_t indice = 0; indice < nodo->hijos.size(); ++indice) {
                const auto &hijo = nodo->hijos[indice];
                cout << " " << (indice + 1) << ". " << hijo->etiqueta << " ("
                          << hijo->indicesEstudiantes.size() << ")\n";
            }
            cout
                << "Seleccione una opcion numerica o presione Enter para terminar en este nivel: ";
            string entrada;
            getline(cin, entrada);
            entrada = recortar(entrada);
            if (entrada.empty()) {
                break;
            }
            try {
                const int opcion = stoi(entrada);
                if (opcion < 1 || opcion > static_cast<int>(nodo->hijos.size())) {
                    throw out_of_range("rango");
                }
                padre = nodo;
                nodo = nodo->hijos[opcion - 1].get();
            } catch (const exception &) {
                cout << "Seleccion invalida. Operacion cancelada.\n";
                return;
            }
        }

        const auto total = static_cast<double>(arbolActual_->indicesEstudiantes.size());
        if (total == 0.0) {
            cout << "No hay estudiantes registrados.\n";
            return;
        }
        const auto cantidadNodo = static_cast<double>(nodo->indicesEstudiantes.size());
        const auto porcentajeTotal = (cantidadNodo / total) * 100.0;
        cout << "\nRuta seleccionada:\n";
        const auto ruta = rutaHastaRaiz(nodo);
        for (size_t i = 1; i < ruta.size(); ++i) {
            const auto *actual = ruta[i];
            const auto nombreVariable = variableComoCadena(actual->variable.value());
            cout << " - " << nombreVariable << ": " << actual->etiqueta << '\n';
        }
        cout << fixed << setprecision(2);
        cout << "\nPorcentaje respecto al total: " << porcentajeTotal << "%\n";
        if (padre != nullptr) {
            const auto porcentajeCondicionado =
                (cantidadNodo / static_cast<double>(padre->indicesEstudiantes.size())) * 100.0;
            cout << "Porcentaje condicionado al nivel anterior: " << porcentajeCondicionado << "%\n";
        } else {
            cout << "Porcentaje condicionado al nivel anterior: 100%\n";
        }
    }

    /**
     * @brief Imprime totales y porcentajes para cada nodo hoja.
     */
    void opcionReporteHojas() {
        if (!arbolListo()) {
            return;
        }
        vector<const NodoArbolClasificacion *> hojas;
        recolectarHojas(*arbolActual_, hojas);
        if (hojas.empty()) {
            cout << "El arbol no tiene hojas.\n";
            return;
        }
        const auto total = static_cast<double>(arbolActual_->indicesEstudiantes.size());
        cout << "\nReporte por hojas:\n";
        cout << fixed << setprecision(2);
        for (const auto *hoja : hojas) {
            const auto ruta = rutaHastaRaiz(hoja);
            ostringstream recorrido;
            for (size_t i = 1; i < ruta.size(); ++i) {
                const auto *nodo = ruta[i];
                const auto nombreVariable = variableComoCadena(nodo->variable.value());
                recorrido << nombreVariable << "=" << nodo->etiqueta;
                if (i + 1 < ruta.size()) {
                    recorrido << " -> ";
                }
            }
            const auto porcentaje = (hoja->indicesEstudiantes.size() / total) * 100.0;
            cout << " - " << recorrido.str() << " | Total: " << hoja->indicesEstudiantes.size()
                      << " | %: " << porcentaje << '\n';
        }
    }

    /**
     * @brief Imprime cada perfil de estudiante incluyendo los datos de historial.
     */
    void opcionImprimirPerfiles() const {
        if (perfiles_.empty()) {
            cout << "No hay estudiantes almacenados.\n";
            return;
        }
        for (const auto &perfil : perfiles_) {
            cout << "\n----------------------------------------\n";
            imprimirPerfil(perfil);
        }
        cout << "\n";
    }

    /**
     * @brief Captura datos para registrar un nuevo estudiante.
     */
    void opcionAgregarEstudiante() {
        cout << "\n=== Registro de estudiante ===\n";
        const auto carne = solicitarNoVacio("Carne");
        if (repositorioEstudiantes_.existe(carne)) {
            cout << "Ya existe un estudiante con ese carne.\n";
            return;
        }
        Estudiante estudiante;
        estudiante.carne = carne;
        estudiante.genero = solicitarNoVacio("Genero");
        estudiante.residencia = solicitarNoVacio("Lugar de residencia");
        estudiante.edad = solicitarEntero("Edad", 1, 110);
        estudiante.colegioProcedencia = solicitarNoVacio("Colegio de procedencia");
        estudiante.tipoColegio = solicitarNoVacio("Tipo de colegio");
        const auto trabaja = solicitarNoVacio("Trabaja (Si/No)");
        const auto trabajaMayusculas = aMayusculas(trabaja);
        estudiante.trabaja = trabajaMayusculas == "SI" || trabajaMayusculas == "S";
        estudiante.estadoCivil = solicitarNoVacio("Estado civil");

        try {
            repositorioEstudiantes_.agregar(estudiante);
            actualizarPerfiles();
            cout << "Estudiante registrado correctamente.\n";
        } catch (const exception &ex) {
            cout << "No se pudo registrar el estudiante: " << ex.what() << '\n';
        }
    }

    /**
     * @brief Captura datos para registrar un nuevo registro de nota.
     */
    void opcionAgregarNota() {
        cout << "\n=== Registro de nota ===\n";
        const auto carne = solicitarNoVacio("Carne del estudiante");
        if (!repositorioEstudiantes_.existe(carne)) {
            cout << "No existe un estudiante con ese carne.\n";
            return;
        }
        RegistroHistorial registro;
        registro.carneEstudiante = carne;
        registro.semestre = solicitarEntero("Semestre (ej. 1, 2)", 1, 20);
        registro.materia = solicitarNoVacio("Materia");
        registro.nota = solicitarDoble("Nota (0-100)", 0.0, 100.0);
        try {
            repositorioHistorial_.agregar(registro);
            actualizarPerfiles();
            cout << "Nota registrada correctamente.\n";
        } catch (const exception &ex) {
            cout << "No se pudo registrar la nota: " << ex.what() << '\n';
        }
    }
};

/**
 * @brief Punto de entrada del programa.
 */
int main() {
    try {
        Aplicacion app;
        app.ejecutar();
    } catch (const exception &ex) {
        cerr << "Error critico: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
