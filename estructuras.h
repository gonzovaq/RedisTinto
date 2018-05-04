    /* typedef enum{
    	ESI = 1,
    	PLANIFICADOR = 2,
    	COORDINADOR = 3,
    	INSTANCIA = 4

    }tTipoDeProceso;

    typedef enum{
    	CONECTARSE = 1,
    	SET = 2,
    	GET = 3,
    	STORE = 4
    }tTipoDeMensaje;


    typedef struct{
    	tTipoDeProceso tipoProceso;
    	tTipoDeMensaje tipoMensaje;
    	int idProceso;
    }tHeader;


    typedef struct{
    	int tamaniooClave;
    	int tamaniooValor;

    }tHeaderInstruccion

    typedef struct{
    	tHeaderInstruccion header;
    	char *clave; ---> Las claves van a tener hasta 40 caracteres
    	char *valor;	---> Los valores no tienen limite
    }
    tInstruccion;

    typedef struct{
    	tHeader encabezado;
    	tInstruccion instruccion;
    }
    tMensajeInstruccion;



    */
