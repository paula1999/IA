#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"

#include <list>
#include <utility>

struct estado {
	int fila;
	int columna;
	int orientacion;
};


class ComportamientoJugador : public Comportamiento {
	public:
		ComportamientoJugador(unsigned int size) : Comportamiento(size) {
			// Inicializar Variables de Estado
			fil = col = 99;
			brujula = 0; // 0: Norte, 1:Este, 2:Sur, 3:Oeste
			destino.fila = -1;
			destino.columna = -1;
			destino.orientacion = -1;
			hayplan = false;
			bikini_on = false;
			recalcular = false;
			num_recalcular = 0;
		}
		
		ComportamientoJugador(std::vector< std::vector< unsigned char> > mapaR) : Comportamiento(mapaR) {
			// Inicializar Variables de Estado
			fil = col = 99;
			brujula = 0; // 0: Norte, 1:Este, 2:Sur, 3:Oeste
			destino.fila = -1;
			destino.columna = -1;
			destino.orientacion = -1;
			hayplan = false;
			bikini_on = false;
			recalcular = false;
			num_recalcular = 0;
		}

		ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
		~ComportamientoJugador(){}
		Action think(Sensores sensores);
		int interact(Action accion, int valor);
		void VisualizaPlan(const estado &st, const list<Action> &plan);
		ComportamientoJugador * clone(){return new ComportamientoJugador(*this);}

	private:
		// Declarar Variables de Estado
		int fil, col, brujula, num_recalcular;
		estado actual, destino;
		list<Action> plan;
		bool 	hayplan,		// Tiene un plan
				bikini_on, 		// Tiene bikini
				zapatillas_on, 	// Tiene zapatillas
				recalcular, 	// Tiene que recalcular un nuevo mapa con menor costo
				recargar;		// Tiene que recargar por tener poca bateria

		// Declarar Variables de costo
		const int Agua = 100, Bosque = 50, Arena = 2, Resto = 1, Agua_b = 10, Bosque_z = 5, desconocido = 15, c_bateria = -5; 

		// Métodos privados de la clase
		bool pathFinding(int level, const estado &origen, const estado &destino, list<Action> &plan);
		bool pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan);
		bool pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan);
		int costo_casilla(char casilla);
		void activar_b(char casilla);
		void activar_z(char casilla);
		void pintarMapa(Sensores sensores);
		bool recarga_cerca(Sensores sensores, estado & recarga);
		bool pathFinding_Costo_Uniforme(const estado &origen, const estado &destino, list<Action> &plan);
		void PintaPlan(list<Action> plan);
		bool HayObstaculoDelante(estado &st);
};

#endif
