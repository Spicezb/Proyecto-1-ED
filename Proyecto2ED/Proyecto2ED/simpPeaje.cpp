#include "simPeaje.h"
#include <algorithm>
#include <cmath>

Simulador::Simulador(const Config& cfgRecibido) {
    cfg = cfgRecibido;
    t = 0.0;
    vehs.clear();
    sigId = 1;
    atendidos = 0;
    rng = std::mt19937_64(123456ULL);
    dist01 = std::uniform_real_distribution<double>(0.0, 1.0);
    distVel = std::uniform_real_distribution<double>(cfg.vMin, cfg.vMax);
    distServicio = std::uniform_real_distribution<double>(cfg.servicioMinSeg, cfg.servicioMaxSeg);
    rrCabina = 0;
    inicializarCabinas();
}

void Simulador::actualizar() {
    intentarGenerarVehiculo();
    moverVehiculos();
    gestionarPeajeConColas();
    atenderCabinas();
    limpiarVehiculosSalidos();
    t += cfg.dt;
}

void Simulador::inicializarCabinas() {
    colas.clear();
    cabinaX.clear();
    servicioRestante.clear();
    atendiendoId.clear();
    if (cfg.numCabinas < 1) {
        cfg.numCabinas = 1;
    }
    colas.resize(static_cast<size_t>(cfg.numCabinas));
    servicioRestante.resize(static_cast<size_t>(cfg.numCabinas), 0.0);
    atendiendoId.resize(static_cast<size_t>(cfg.numCabinas), 0);
    if (cfg.numCarriles <= 1) {
        for (int k = 0; k < cfg.numCabinas; k++) {
            cabinaX.push_back(static_cast<float>(cfg.carrilInicioX));
        }
        return;
    }
    float xMin = static_cast<float>(cfg.carrilInicioX);
    float xMax = static_cast<float>(cfg.carrilInicioX + (cfg.numCarriles - 1) * cfg.sepCarrilX);
    if (cfg.numCabinas == 1) {
        cabinaX.push_back((xMin + xMax) * 0.5f);
        return;
    }
    for (int k = 0; k < cfg.numCabinas; k++) {
        float tpos = static_cast<float>(k) / static_cast<float>(cfg.numCabinas - 1);
        float x = xMin + tpos * (xMax - xMin);
        cabinaX.push_back(x);
    }
}

void Simulador::intentarGenerarVehiculo() {
    double probabilidadFrame = cfg.probLlegadaPorSeg * cfg.dt;
    if (dist01(rng) < probabilidadFrame) {
        Vehiculo v;
        v.id = sigId++;
        v.carril = static_cast<int>(dist01(rng) * cfg.numCarriles);
        if (v.carril < 0) {
            v.carril = 0;
        }
        if (v.carril >= cfg.numCarriles) {
            v.carril = cfg.numCarriles - 1;
        }
        v.x = cfg.carrilInicioX + v.carril * cfg.sepCarrilX;
        v.y = cfg.entradaY;
        v.vDeseada = distVel(rng);
        v.v = v.vDeseada * 0.8;
        v.cabinaObjetivo = -1;
        v.xObjetivo = v.x;
        v.estado = EstadoVehiculo::EnRuta;
        vehs.push_back(v);
    }
}

int Simulador::elegirCabinaLibreMasCercana(double xVeh) const {
    int mejor = -1;
    double mejorD = 0.0;
    for (int k = 0; k < static_cast<int>(cabinaX.size()); k++) {
        bool libre = servicioRestante[static_cast<size_t>(k)] == 0.0 && colas[static_cast<size_t>(k)].empty() && atendiendoId[static_cast<size_t>(k)] == 0;
        if (libre) {
            double d = std::fabs(xVeh - static_cast<double>(cabinaX[static_cast<size_t>(k)]));
            if (mejor == -1 || d < mejorD) {
                mejor = k;
                mejorD = d;
            }
        }
    }
    return mejor;
}

int Simulador::elegirCabinaMinColaRoundRobin() {
    if (colas.empty()) {
        return 0;
    }
    size_t minTam = colas[0].size();
    for (size_t k = 1; k < colas.size(); k++) {
        if (colas[k].size() < minTam) {
            minTam = colas[k].size();
        }
    }
    std::vector<int> indicesMin;
    indicesMin.reserve(colas.size());
    for (int k = 0; k < static_cast<int>(colas.size()); k++) {
        if (colas[static_cast<size_t>(k)].size() == minTam) {
            indicesMin.push_back(k);
        }
    }
    if (indicesMin.empty()) {
        return 0;
    }
    int elegido = indicesMin[static_cast<size_t>(rrCabina % indicesMin.size())];
    rrCabina = rrCabina + 1;
    return elegido;
}

void Simulador::fijarCabinaObjetivoSiHaceFalta(Vehiculo& v) {
    if (v.cabinaObjetivo != -1) {
        return;
    }
    if (v.y >= cfg.zonaDecisionY) {
        int kLibre = elegirCabinaLibreMasCercana(v.x);
        int kDestino = kLibre != -1 ? kLibre : elegirCabinaMinColaRoundRobin();
        v.cabinaObjetivo = kDestino;
        v.xObjetivo = static_cast<double>(cabinaX[static_cast<size_t>(kDestino)]);
    }
}

void Simulador::avanzarXLateralHaciaObjetivo(Vehiculo& v) {
    if (v.cabinaObjetivo == -1) {
        return;
    }
    double dx = v.xObjetivo - v.x;
    if (std::fabs(dx) < 1e-3) {
        v.x = v.xObjetivo;
        return;
    }
    double paso = cfg.vLateral * cfg.dt;
    if (std::fabs(dx) <= paso) {
        v.x = v.xObjetivo;
    }
    else {
        v.x = v.x + (dx > 0.0 ? paso : -paso);
    }
}

void Simulador::moverVehiculos() {
    for (auto& v : vehs) {
        if (v.estado == EstadoVehiculo::EnRuta) {
            v.v += (v.vDeseada - v.v) * 0.05;
            v.y += v.v * cfg.dt;
            fijarCabinaObjetivoSiHaceFalta(v);
            avanzarXLateralHaciaObjetivo(v);
        }
        else if (v.estado == EstadoVehiculo::EnCola || v.estado == EstadoVehiculo::Atendido) {
            v.v = 0.0;
        }
    }
}

void Simulador::gestionarPeajeConColas() {
    for (auto& v : vehs) {
        if (v.estado == EstadoVehiculo::EnRuta && v.y >= cfg.posPeajeY) {
            v.estado = EstadoVehiculo::EnCola;
            v.y = cfg.posPeajeY;
            int destino = v.cabinaObjetivo;
            if (destino < 0 || destino >= static_cast<int>(cabinaX.size())) {
                destino = elegirCabinaMinColaRoundRobin();
            }
            asignarACabina(v.id, destino);
        }
    }
}

void Simulador::asignarACabina(IdVehiculo idVeh, int cabinaIdx) {
    if (cabinaIdx < 0 || cabinaIdx >= static_cast<int>(colas.size())) {
        cabinaIdx = 0;
    }
    colas[static_cast<size_t>(cabinaIdx)].push_back(idVeh);
    int idx = indiceVehiculoPorId(idVeh);
    if (idx >= 0) {
        if (cabinaIdx >= 0 && cabinaIdx < static_cast<int>(cabinaX.size())) {
            vehs[static_cast<size_t>(idx)].x = cabinaX[static_cast<size_t>(cabinaIdx)];
        }
        vehs[static_cast<size_t>(idx)].estado = EstadoVehiculo::EnCola;
    }
}

int Simulador::indiceVehiculoPorId(IdVehiculo id) const {
    for (int i = 0; i < static_cast<int>(vehs.size()); i++) {
        if (vehs[static_cast<size_t>(i)].id == id) {
            return i;
        }
    }
    return -1;
}

void Simulador::atenderCabinas() {
    for (int k = 0; k < static_cast<int>(colas.size()); k++) {
        if (servicioRestante[static_cast<size_t>(k)] > 0.0) {
            servicioRestante[static_cast<size_t>(k)] = servicioRestante[static_cast<size_t>(k)] - cfg.dt;
            if (servicioRestante[static_cast<size_t>(k)] <= 0.0) {
                if (atendiendoId[static_cast<size_t>(k)] != 0) {
                    int idx = indiceVehiculoPorId(atendiendoId[static_cast<size_t>(k)]);
                    if (idx >= 0) {
                        vehs[static_cast<size_t>(idx)].estado = EstadoVehiculo::Salio;
                        atendidos = atendidos + 1;
                    }
                }
                atendiendoId[static_cast<size_t>(k)] = 0;
                servicioRestante[static_cast<size_t>(k)] = 0.0;
            }
        }
        if (servicioRestante[static_cast<size_t>(k)] == 0.0 && atendiendoId[static_cast<size_t>(k)] == 0) {
            if (!colas[static_cast<size_t>(k)].empty()) {
                IdVehiculo idVeh = colas[static_cast<size_t>(k)].front();
                colas[static_cast<size_t>(k)].erase(colas[static_cast<size_t>(k)].begin());
                atendiendoId[static_cast<size_t>(k)] = idVeh;
                servicioRestante[static_cast<size_t>(k)] = distServicio(rng);
                int idx = indiceVehiculoPorId(idVeh);
                if (idx >= 0) {
                    vehs[static_cast<size_t>(idx)].estado = EstadoVehiculo::Atendido;
                }
            }
        }
    }
}

void Simulador::limpiarVehiculosSalidos() {
    vehs.erase(std::remove_if(vehs.begin(), vehs.end(), [&](const Vehiculo& v) { return v.estado == EstadoVehiculo::Salio; }), vehs.end());
}

