#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"

#include <iostream>
#include <cmath>
#include <set>
#include <stack>
#include <queue>
#include <utility>

// Paula Villanueva Nuñez


// Este es el método principal que debe contener los 4 Comportamientos_Jugador
// que se piden en la práctica. Tiene como entrada la información de los
// sensores y devuelve la acción a realizar.
Action ComportamientoJugador::think(Sensores sensores) {
	Action accion = actIDLE;			// Siguiente accion
	estado recarga;						// Lugar de la casilla de recarga
	const int	MAX_BATERIA = 3000,		// Maxima bateria posible
				LIM_BATERIA = 2700,		// Maximo de bateria para parar a recargar
				LIM_VIDA = 450;			// Minimo de vida para parar a recargar

	actual.fila        = sensores.posF;
	actual.columna     = sensores.posC;
	actual.orientacion = sensores.sentido;
	fil = sensores.posF;
	col = sensores.posC;

	//cout << "Fila: " << actual.fila << endl;
	//cout << "Col : " << actual.columna << endl;
	//cout << "Ori : " << actual.orientacion << endl;

	destino.fila       = sensores.destinoF;
	destino.columna    = sensores.destinoC;
	
	// Nivel 1
	if (sensores.nivel != 4){
		// Si no hay plan, recalcular uno nuevo
		if (!hayplan)	
			hayplan = pathFinding (sensores.nivel, actual, destino, plan);
		
		
		if ((sensores.terreno[2]=='P' or sensores.terreno[2]=='M' or sensores.superficie[2]=='a') and plan.front() == actFORWARD)
			accion = actTURN_R;
		// Si nos cambian el objetivo, recalcular nuevo plan
		else if (plan.size() == 0)	
			hayplan = false;
		else if (hayplan and plan.size()>0){
			accion = plan.front();
			plan.erase(plan.begin());
		}
		else
			accion = actFORWARD;
	}
	else{
		// Salir del modo de recarga cuando alcancemos el maximo de bateria o quede poco tiempo
		if (sensores.bateria == MAX_BATERIA or sensores.vida <= LIM_VIDA)	
			recargar = false;

		// Si hay una recarga cerca y tenemos poca bateria y suficiente tiempo, ir y recargar
		if (recarga_cerca(sensores, recarga) and !recargar and sensores.bateria < LIM_BATERIA and sensores.vida > LIM_VIDA){
			hayplan = pathFinding (sensores.nivel, actual, recarga, plan);
			recargar = true;
		}
		// Si no estamos recargando y no hay plan o necesitamos recalcular, calculamos un nuevo plan
		else if ((!hayplan or recalcular) and !recargar){
			hayplan = pathFinding (sensores.nivel, actual, destino, plan);
			recalcular = true;
		}

		// Añado en el mapa la información que conozco
		pintarMapa(sensores);

		// Si me voy a cruzar con un aldeano, esperar a que se vaya
		if (sensores.superficie[2] == 'a' and plan.front() == actFORWARD)
			accion = actIDLE;
		// Si me voy a chocar con un muro o caer por un precipicio, girar y calcular nuevo plan
		else if ((sensores.terreno[2] == 'P' or sensores.terreno[2] == 'M') and plan.front() == actFORWARD){
			accion = actTURN_R;
			hayplan = false;
		}
		// Si estoy en una casilla de recarga y tengo poca bateria y tiempo suficiente, paro y recargo
		else if (mapaResultado[actual.fila][actual.columna] == 'X' and sensores.bateria < MAX_BATERIA and sensores.vida > LIM_VIDA and recargar){
			accion = actIDLE;
		}
		// Si la siguiente accion me lleva a un terreno que cuesta mas de 50 (bosque o agua), recalcular nuevo plan
		else if (costo_casilla(sensores.terreno[2]) >= 50 and plan.front() == actFORWARD and !recalcular and num_recalcular < 3){
			accion = actIDLE;
			recalcular = true;
			num_recalcular++;

			if (num_recalcular >= 3){
				recalcular = false;
				accion = plan.front();
				plan.erase(plan.begin());
			}
		}
		else if (hayplan and plan.size()>0){
			accion = plan.front();

			if (recalcular){
				// Si estamos cerca del objetivo, no recalcular
				if (plan.size() < 7)
					recalcular = false;
				// No recalcular para evitar entrar en bucle entre acciones de girar
				else if (accion == actTURN_R or accion == actTURN_L)
					recalcular = false;
				// Si la siguiente accion no me lleva a un terreno que cuesta mas de 50, no recalcular
				else if (!(costo_casilla(sensores.terreno[2]) >= 50 and plan.front() == actFORWARD))
					recalcular = false;
				else if (num_recalcular >= 3)
					recalcular = false;
			}

			if (recalcular){
				accion = actIDLE;
				num_recalcular++;
			}
			else{
				plan.erase(plan.begin());
				num_recalcular = 0;
			}	
		}
		// Si hemos terminado el plan, recalcular otro
		else if (plan.size() == 0){
			accion = actIDLE;
			hayplan = false;
		}
		else
			accion = actFORWARD;
	}

	return accion;
}

/**
 * @brief Pinta el mapa con la información recibida de los sensores
 * @param sensores Sensores que reciben la información cercana al robot
 */
void ComportamientoJugador::pintarMapa(Sensores sensores){
	int k;

	switch (sensores.sentido){
			case norte:
				brujula = 0;
				k = 0;

				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1); j++){
						mapaResultado[actual.fila - i][actual.columna - i + j] = sensores.terreno[k];
						k++;
					}
				}

				break;
			case este:
				brujula = 1;
				k = 0;

				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1); j++){
						mapaResultado[actual.fila - i + j][actual.columna + i] = sensores.terreno[k];
						k++;
					}
				}

				break;
			case sur:
				brujula = 2;
				k = 0;

				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1); j++){
						mapaResultado[actual.fila + i][actual.columna + i - j] = sensores.terreno[k];
						k++;
					}
				}

				break;
			case oeste: 
				brujula = 3;
				k = 0;

				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1); j++){
						mapaResultado[actual.fila + i - j][actual.columna - i] = sensores.terreno[k];
						k++;
					}
				}

				break;
		}
}

/**
 * @brief Comprueba si hay una casilla de recarga cerca
 * @param sensores Sensores que perciben la información cerca del robot
 * @recarga Casilla donde se almacena el lugar de la casilla de recarga, si la hay
 * @return Devuelve true si hay una casilla de recarga cerca, devuelve false si no
 */
bool ComportamientoJugador::recarga_cerca (Sensores sensores, estado & recarga){
	int k = 0;
	bool salir = false;

	switch (sensores.sentido){
			case norte:
				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1) and !salir; j++){
						if (sensores.terreno[k] == 'X'){
							recarga.fila = actual.fila - i;
							recarga.columna = actual.columna - i + j;
							salir = true;
						}

						k++;
					}
				}

				break;
			case este:
				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1) and !salir; j++){
						if (sensores.terreno[k] == 'X'){
							recarga.fila = actual.fila - i + j;
							recarga.columna = actual.columna + i;
							salir = true;
						}

						k++;
					}
				}

				break;
			case sur:
				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1) and !salir; j++){
						if (sensores.terreno[k] == 'X'){
							recarga.fila = actual.fila + i;
							recarga.columna = actual.columna + i - j;
							salir = true;
						}

						k++;
					}
				}

				break;
			case oeste: 
				for (int i = 0; i < 4; i++){
					for (int j = 0; j < (2*i + 1) and !salir; j++){
						if (sensores.terreno[k] == 'X'){
							recarga.fila = actual.fila + i - j;
							recarga.columna = actual.columna - i;
							salir = true;
						}

						k++;
					}
				}

				break;
		}

	return salir;
}


// Llama al algoritmo de busqueda que se usará en cada comportamiento del agente
// Level representa el comportamiento en el que fue iniciado el agente.
bool ComportamientoJugador::pathFinding (int level, const estado &origen, const estado &destino, list<Action> &plan){
	switch (level){
		case 1:
			cout << "Busqueda en profundad\n";
			
			return pathFinding_Profundidad(origen,destino,plan);
			break;
		case 2:
			cout << "Busqueda en Anchura\n";

			return pathFinding_Anchura(origen, destino, plan);
			break;
		case 3:
			cout << "Busqueda Costo Uniforme\n";
			
			return pathFinding_Costo_Uniforme(origen, destino, plan);
			break;
		case 4: 
			cout << "Busqueda para el reto\n";

			return pathFinding_Costo_Uniforme(origen, destino, plan);
			break;
	}

	cout << "Comportamiento sin implementar\n";
	
	return false;
}


//---------------------- Implementación de la busqueda en profundidad ---------------------------

// Dado el código en carácter de una casilla del mapa dice si se puede
// pasar por ella sin riegos de morir o chocar.
bool EsObstaculo (unsigned char casilla){
	if (casilla=='P' or casilla=='M')
		return true;
	else
	  return false;
}


// Comprueba si la casilla que hay delante es un obstaculo. Si es un
// obstaculo devuelve true. Si no es un obstaculo, devuelve false y
// modifica st con la posición de la casilla del avance.
bool ComportamientoJugador::HayObstaculoDelante(estado &st){
	int fil = st.fila, col = st.columna;

  	// calculo cual es la casilla de delante del agente
	switch (st.orientacion) {
		case 0: fil--; break;
		case 1: col++; break;
		case 2: fil++; break;
		case 3: col--; break;
	}

	// Compruebo que no me salgo fuera del rango del mapa
	if (fil<0 or fil>=mapaResultado.size()) return true;
	if (col<0 or col>=mapaResultado[0].size()) return true;

	// Miro si en esa casilla hay un obstaculo infranqueable
	if (!EsObstaculo(mapaResultado[fil][col])){
		// No hay obstaculo, actualizo el parámetro st poniendo la casilla de delante.
    	st.fila = fil;
		st.columna = col;
		
		return false;
	}
	else{
	  return true;
	}
}

struct nodo{
	estado st;
	list<Action> secuencia;
	int coste = 0;
	int destinoF = -1;
	int destinoC = -1;

	// Coste de usar el nodo
	int coste_nodo (int destF, int destC) const{
		int dist = abs(st.fila-destF) + abs(st.columna-destC);
		int total = dist * coste;
		
		return total;
	}

	// Criterio para ordenar la cola de prioridad del algoritmo
	bool operator<(const nodo & n) const{
		return (n.coste_nodo(n.destinoF, n.destinoC) < coste_nodo(destinoF, destinoC));
	}
};

struct ComparaEstados{
	bool operator()(const estado &a, const estado &n) const{
		if ((a.fila > n.fila) or (a.fila == n.fila and a.columna > n.columna) or
	      (a.fila == n.fila and a.columna == n.columna and a.orientacion > n.orientacion))
			return true;
		else
			return false;
	}
};


// Implementación de la búsqueda en profundidad.
// Entran los puntos origen y destino y devuelve la
// secuencia de acciones en plan, una lista de acciones.
bool ComportamientoJugador::pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	cout << "Calculando plan\n";
	
	plan.clear();
	
	set<estado,ComparaEstados> generados; // Lista de Cerrados
	stack<nodo> pila; // Lista de Abiertos
	nodo current;
	
	current.st = origen;
	current.secuencia.empty();

	pila.push(current);

  	while (!pila.empty() and (current.st.fila != destino.fila or current.st.columna != destino.columna)){
		pila.pop();
		generados.insert(current.st);

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;

		if (generados.find(hijoTurnR.st) == generados.end()){
			hijoTurnR.secuencia.push_back(actTURN_R);
			pila.push(hijoTurnR);
		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;

		if (generados.find(hijoTurnL.st) == generados.end()){
			hijoTurnL.secuencia.push_back(actTURN_L);
			pila.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;

		if (!HayObstaculoDelante(hijoForward.st)){
			if (generados.find(hijoForward.st) == generados.end()){
				hijoForward.secuencia.push_back(actFORWARD);
				pila.push(hijoForward);
			}
		}

		// Tomo el siguiente valor de la pila
		if (!pila.empty()){
			current = pila.top();
		}
	}

  	cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		
		plan = current.secuencia;
		
		cout << "Longitud del plan: " << plan.size() << endl;
		
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}

	return false;
}




//---------------------- Implementación de la busqueda en anchura ---------------------------

// Implementación de la búsqueda en anchura.
// Entran los puntos origen y destino y devuelve la
// secuencia de acciones en plan, una lista de acciones.
bool ComportamientoJugador::pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	cout << "Calculando plan\n";
	
	plan.clear();
	
	set<estado,ComparaEstados> generados; // Lista de Cerrados
	queue<nodo> cola; // Lista de Abiertos
	nodo current;
	
	current.st = origen;
	current.secuencia.empty();

	cola.push(current);

  	while (!cola.empty() and (current.st.fila != destino.fila or current.st.columna != destino.columna)){
		cola.pop();
		generados.insert(current.st);

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;

		if (generados.find(hijoTurnR.st) == generados.end()){
			hijoTurnR.secuencia.push_back(actTURN_R);
			cola.push(hijoTurnR);
		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;

		if (generados.find(hijoTurnL.st) == generados.end()){
			hijoTurnL.secuencia.push_back(actTURN_L);
			cola.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;

		if (!HayObstaculoDelante(hijoForward.st)){
			if (generados.find(hijoForward.st) == generados.end()){
				hijoForward.secuencia.push_back(actFORWARD);
				cola.push(hijoForward);
			}
		}

		// Tomo el siguiente valor de la cola
		if (!cola.empty()){
			current = cola.front();
		}
	}

  	cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		
		plan = current.secuencia;
		
		cout << "Longitud del plan: " << plan.size() << endl;
		
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}

	return false;
}

//---------------------- Implementación de la busqueda con costo uniforme ---------------------------

/** @brief Devuelve el coste de una casilla
 * @param casilla De quien se quiere calcular el coste
 * @return Devuelve el coste de la casilla
 */ 
int ComportamientoJugador::costo_casilla(char casilla){
	switch (casilla){
		case 'A':
			if (bikini_on)
				return Agua_b; // 10
			return Agua; // 100
			break;
		case 'B':
			if (zapatillas_on)
				return Bosque_z; // 5
			return Bosque; // 50
			break;
		case 'T':
			return Arena; // 2
			break;
		case '?':
			return desconocido; // 15
			break;
		case 'X':
			return c_bateria;	// -5
		default:
			return Resto; // 1
			break;
	}
}

/** @brief Se activa el bikini si pasamos por la casilla de bikini
 * @param casilla Lugar por donde pasamos y evaluamos si tiene bikini
 */
void ComportamientoJugador::activar_b(char casilla){
	if (casilla == 'K')
		bikini_on = true;
}
/** @brief Se activan las zapatillas si pasamos por la casilla de zapatillas
 * @param casilla Lugar por donde pasamos y evaluamos si tiene zapatillas
 */
void ComportamientoJugador::activar_z(char casilla){
	if (casilla == 'D')
		zapatillas_on = true;
}

// Implementación de la búsqueda con costo uniforme.
// Entran los puntos origen y destino y devuelve la
// secuencia de acciones en plan, una lista de acciones.
bool ComportamientoJugador::pathFinding_Costo_Uniforme(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	cout << "Calculando plan\n";
	
	plan.clear();
	
	set<estado,ComparaEstados> generados; // Lista de Cerrados (nodos expandidos)
	priority_queue<nodo> lista; // Lista de Abiertos (nodos no expandidos)
	nodo current;
	int mejor_costo;
	
	current.st = origen;
	current.secuencia.empty();
	current.destinoF = destino.fila;
	current.destinoC = destino.columna;
	
	lista.push(current);

  	while (!lista.empty() and (current.st.fila != destino.fila or current.st.columna != destino.columna)){
		generados.insert(current.st);

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;

		if (generados.find(hijoTurnR.st) == generados.end()){
			hijoTurnR.secuencia.push_back(actTURN_R);
			hijoTurnR.coste += costo_casilla(mapaResultado[hijoTurnR.st.fila][hijoTurnR.st.columna]);
			lista.push(hijoTurnR);
		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;

		if (generados.find(hijoTurnL.st) == generados.end()){
			hijoTurnL.secuencia.push_back(actTURN_L);
			hijoTurnL.coste += costo_casilla(mapaResultado[hijoTurnL.st.fila][hijoTurnL.st.columna]);
			lista.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;

		if (!HayObstaculoDelante(hijoForward.st)){
			if (generados.find(hijoForward.st) == generados.end()){
				hijoForward.secuencia.push_back(actFORWARD);
				hijoForward.coste += costo_casilla(mapaResultado[hijoForward.st.fila][hijoForward.st.columna]);
				lista.push(hijoForward);
			}
		}

		// Tomo el siguiente valor con costo minimo de la lista
		if (!lista.empty()){
			current = lista.top();

			// Activo bikini si no lo tengo y paso por su casilla
			if (!bikini_on)
				activar_b(mapaResultado[current.st.fila][current.st.columna]);
			
			// Activo las zapatillas si no las tengo y paso por su casilla
			if (!zapatillas_on)
				activar_z(mapaResultado[current.st.fila][current.st.columna]);
			
			lista.pop();
		}
	}

  	cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		
		plan = current.secuencia;
		
		cout << "Longitud del plan: " << plan.size() << endl;
		
		PintaPlan(plan);
		VisualizaPlan(origen, plan);	// ver el plan en el mapa
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}

	return false;
}


// Sacar por la términal la secuencia del plan obtenido
void ComportamientoJugador::PintaPlan(list<Action> plan) {
	auto it = plan.begin();
	
	while (it!=plan.end()){
		if (*it == actFORWARD){
			cout << "A ";
		}
		else if (*it == actTURN_R){
			cout << "D ";
		}
		else if (*it == actTURN_L){
			cout << "I ";
		}
		else {
			cout << "- ";
		}
		it++;
	}

	cout << endl;
}



void AnularMatriz(vector<vector<unsigned char> > &m){
	for (int i=0; i<m[0].size(); i++){
		for (int j=0; j<m.size(); j++){
			m[i][j]=0;
		}
	}
}


// Pinta sobre el mapa del juego el plan obtenido
void ComportamientoJugador::VisualizaPlan(const estado &st, const list<Action> &plan){
  	AnularMatriz(mapaConPlan);
	estado cst = st;
	auto it = plan.begin();
	
	while (it != plan.end()){
		if (*it == actFORWARD){
			switch (cst.orientacion) {
				case 0: cst.fila--; break;
				case 1: cst.columna++; break;
				case 2: cst.fila++; break;
				case 3: cst.columna--; break;
			}
			mapaConPlan[cst.fila][cst.columna]=1;
		}
		else if (*it == actTURN_R){
			cst.orientacion = (cst.orientacion+1)%4;
		}
		else {
			cst.orientacion = (cst.orientacion+3)%4;
		}

		it++;
	}
}



int ComportamientoJugador::interact(Action accion, int valor){
  	return false;
}
