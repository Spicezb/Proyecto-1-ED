#pragma once
#include <vector>
#include <random>
#include <cstdint>

using IdVehiculo = std::uint64_t;

enum class EstadoVehiculo : std::uint8_t {
    EnRuta,
    EnCola,
    Atendido,
    Salio
};

struct Vehiculo {
    IdVehiculo id = 0;
    int carril = 0;
    double x = 0.0;
    double y = 0.0;
    double v = 0.0;
    double vDeseada = 110.0;
    double largo = 40.0;
    double ancho = 18.0;
    int cabinaObjetivo = -1;       
    double xObjetivo = 0.0;         
    EstadoVehiculo estado = EstadoVehiculo::EnRuta;
};

struct Config {
    int anchoPantalla = 1280;
    int altoPantalla = 720;
    int numCarriles = 3;
    int numCabinas = 5;
    double dt = 1.0 / 60.0;
    double probLlegadaPorSeg = 0.5;
    double vMin = 80.0;
    double vMax = 140.0;
    double posPeajeY = 600.0;
    double carrilInicioX = 100.0;
    double sepCarrilX = 120.0;
    double entradaY = 20.0;
    double servicioMinSeg = 2.0;
    double servicioMaxSeg = 5.0;
    double vLateral = 220.0;        
    double zonaDecisionY = 520.0;   
};

class Simulador {
public:
    explicit Simulador(const Config& cfgRecibido);
    void actualizar();
    const std::vector<Vehiculo>& getVehiculos() const {
        return vehs;
    }
    const Config& getConfig() const {
        return cfg;
    }
    double getTiempo() const {
        return t;
    }
    int getAtendidos() const {
        return atendidos;
    }
private:
    Config cfg;
    double t = 0.0;
    std::mt19937_64 rng;
    std::uniform_real_distribution<double> dist01;
    std::uniform_real_distribution<double> distVel;
    std::uniform_real_distribution<double> distServicio;
    std::vector<Vehiculo> vehs;
    IdVehiculo sigId = 1;
    int atendidos = 0;
    std::vector<std::vector<IdVehiculo>> colas;
    std::vector<float> cabinaX;
    std::vector<double> servicioRestante;
    std::vector<IdVehiculo> atendiendoId;
    int rrCabina = 0;
    void intentarGenerarVehiculo();
    void moverVehiculos();
    void gestionarPeajeConColas();
    void inicializarCabinas();
    void asignarACabina(IdVehiculo idVeh, int cabinaIdx); 
    int indiceVehiculoPorId(IdVehiculo id) const;
    void atenderCabinas();
    void limpiarVehiculosSalidos();
    int elegirCabinaLibreMasCercana(double xVeh) const;    
    int elegirCabinaMinColaRoundRobin();                  
    void fijarCabinaObjetivoSiHaceFalta(Vehiculo& v);     
    void avanzarXLateralHaciaObjetivo(Vehiculo& v);        
};
