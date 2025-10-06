# Especificación del Protocolo de Comunicación
## Sistema de Telemetría de Vehículo Autónomo (AVTS)

---

## 1. Introducción

El Sistema de Telemetría de Vehículo Autónomo (AVTS) implementa un protocolo de comunicación cliente-servidor basado en texto plano que permite la transmisión de datos de telemetría en tiempo real y el control remoto de un vehículo autónomo simulado. El protocolo está diseñado para soportar múltiples clientes concurrentes con diferentes niveles de acceso y privilegios.

### 1.1 Objetivos del Protocolo

- **Transmisión de Telemetría**: Envío periódico de datos del vehículo a todos los clientes conectados
- **Control Remoto**: Permitir comandos de control desde clientes autorizados
- **Autenticación Segura**: Sistema de login con roles diferenciados
- **Concurrencia**: Soporte para múltiples clientes simultáneos (hasta 100)
- **Seguridad**: Protección contra accesos no autorizados y ataques de fuerza bruta

### 1.2 Características Principales

- Protocolo basado en texto ASCII
- Comunicación sobre TCP/IP
- Autenticación con hash SHA-256 y salt
- Sistema de roles (ADMIN/OBSERVER)
- Transmisión broadcast de telemetría cada 10 segundos
- Manejo robusto de errores y desconexiones

---

## 2. Arquitectura del Protocolo

### 2.1 Modelo Cliente-Servidor

El protocolo AVTS sigue un modelo cliente-servidor donde:

- **Servidor**: Mantiene el estado del vehículo y gestiona las conexiones
- **Clientes**: Se conectan para recibir telemetría y/o enviar comandos
- **Comunicación**: Bidireccional asíncrona sobre TCP

### 2.2 Estados de Conexión

Cada cliente pasa por los siguientes estados:

1. **CONECTADO**: Cliente conectado pero no autenticado
2. **AUTENTICADO**: Cliente con credenciales válidas
3. **ACTIVO**: Cliente recibiendo telemetría y/o enviando comandos
4. **BLOQUEADO**: Cliente temporalmente bloqueado por intentos fallidos
5. **DESCONECTADO**: Conexión terminada

### 2.3 Flujo de Comunicación

```
Cliente                    Servidor
  |                          |
  |---- TCP Connect -------->|
  |<--- Connection OK -------|
  |                          |
  |---- LOGIN comando ------>|
  |<--- OK/ERR response -----|
  |                          |
  |<--- DATA broadcast ------|  (cada 10s)
  |---- Comandos ----------->|
  |<--- Respuestas ----------|
  |                          |
  |---- Disconnect --------->|
```

---

## 3. Formato de Mensajes

### 3.1 Estructura General

Todos los mensajes siguen la estructura:
```
<COMANDO> [<PARÁMETROS>]\n
```

**Características:**
- Terminación obligatoria con `\n` (Line Feed)
- Codificación ASCII
- Máximo 512 bytes por mensaje
- Insensible a mayúsculas/minúsculas para comandos
- Espacios como separadores de parámetros

### 3.2 Tipos de Mensajes

#### 3.2.1 Mensajes de Autenticación

**LOGIN - Autenticación de Cliente**
```
LOGIN <ROLE> <USERNAME> <PASSWORD>\n
```

**Parámetros:**
- `ROLE`: "ADMIN" o "OBSERVER"
- `USERNAME`: Nombre de usuario (máx. 31 caracteres)
- `PASSWORD`: Contraseña en texto plano (máx. 63 caracteres)

**Ejemplo:**
```
LOGIN ADMIN root password\n
LOGIN OBSERVER juan 12345\n
```

#### 3.2.2 Mensajes de Comando (Solo ADMIN)

**SPEED UP - Incrementar Velocidad**
```
SPEED UP\n
```
- Incrementa el offset de velocidad en 1.0 unidad
- Solo disponible para usuarios ADMIN

**TURN LEFT - Girar a la Izquierda**
```
TURN LEFT\n
```
- Cambia la dirección del vehículo a LEFT
- Solo disponible para usuarios ADMIN

**TURN RIGHT - Girar a la Derecha**
```
TURN RIGHT\n
```
- Cambia la dirección del vehículo a RIGHT
- Solo disponible para usuarios ADMIN

**BATTERY RESET - Resetear Batería**
```
BATTERY RESET\n
```
- Restablece el nivel de batería al 100%
- Solo disponible para usuarios ADMIN

#### 3.2.3 Mensajes de Consulta (ADMIN y OBSERVER)

**STATUS - Consultar Estado del Vehículo**
```
STATUS\n
```
- Solicita el estado actual del vehículo
- Disponible para ambos roles

#### 3.2.4 Mensajes de Telemetría (Servidor → Cliente)

**DATA - Datos de Telemetría**
```
DATA <SPEED> <BATTERY> <DIRECTION> <TEMPERATURE>\n
```

**Parámetros:**
- `SPEED`: Velocidad actual (float, 0.0-∞)
- `BATTERY`: Nivel de batería (int, 0-100)
- `DIRECTION`: Dirección actual ("FORWARD", "LEFT", "RIGHT", "STOP")
- `TEMPERATURE`: Temperatura en Celsius (int, 20-40)

**Ejemplo:**
```
DATA 75.5 85 FORWARD 28\n
DATA 120.3 60 LEFT 32\n
```

#### 3.2.5 Mensajes de Respuesta del Servidor

**OK - Comando Ejecutado Exitosamente**
```
OK\n
```

**ERR - Mensajes de Error**
```
ERR <TIPO_ERROR>\n
```

**Tipos de Error:**
- `ERR locked\n`: Cuenta bloqueada por intentos fallidos
- `ERR invalid (X/3)\n`: Credenciales inválidas (X = número de intento)
- `ERR observer_only\n`: Observer intentó usar comando de ADMIN
- `ERR unknown_cmd\n`: Comando no reconocido

---

## 4. Reglas de Procedimiento

### 4.1 Establecimiento de Conexión

1. **Conexión TCP**: Cliente establece conexión TCP con el servidor
2. **Estado Inicial**: Cliente queda en estado "CONECTADO" pero no autenticado
3. **Timeout**: Conexiones sin autenticar se cierran después de 60 segundos
4. **Límite de Clientes**: Máximo 100 clientes simultáneos

### 4.2 Proceso de Autenticación

#### 4.2.1 Flujo Normal de Login

1. Cliente envía comando `LOGIN`
2. Servidor valida credenciales usando hash SHA-256 + salt
3. Si es válido: responde `OK\n` y marca cliente como autenticado
4. Si es inválido: responde `ERR invalid (X/3)\n` donde X es el número de intento

#### 4.2.2 Manejo de Intentos Fallidos

- **Límite**: 3 intentos fallidos consecutivos
- **Bloqueo**: Después del 3er intento, cuenta se bloquea por 5 minutos
- **Respuesta**: `ERR locked\n` durante el período de bloqueo
- **Reset**: Contador se reinicia después de login exitoso o expiración del bloqueo

#### 4.2.3 Validación de Credenciales

```
hash_calculado = SHA256(salt_usuario + password_plano)
if (hash_calculado == hash_almacenado) {
    autenticación_exitosa();
} else {
    incrementar_intentos_fallidos();
}
```

### 4.3 Transmisión de Telemetría

#### 4.3.1 Broadcast Periódico

- **Frecuencia**: Cada 10 segundos exactos
- **Destinatarios**: Todos los clientes autenticados
- **Formato**: Mensaje `DATA` con valores actuales
- **Asíncrono**: No requiere solicitud del cliente

#### 4.3.2 Generación de Datos

**Velocidad:**
```
velocidad_final = velocidad_base_aleatoria + speed_offset
if (velocidad_final < 0) velocidad_final = 0
```

**Batería:**
```
battery_level -= 5  // cada 10 segundos
if (battery_level <= 0) battery_level = 100  // auto-reset
```

**Temperatura:**
```
temperatura = 20 + (random() % 21)  // 20-40°C
```

### 4.4 Procesamiento de Comandos

#### 4.4.1 Validación de Comandos

1. **Autenticación**: Cliente debe estar autenticado
2. **Autorización**: Verificar permisos según rol
3. **Formato**: Validar sintaxis del comando
4. **Ejecución**: Aplicar cambios al estado del vehículo
5. **Respuesta**: Enviar confirmación o error

#### 4.4.2 Control de Acceso por Roles

**ADMIN (Administrador):**
- Todos los comandos disponibles
- Puede modificar estado del vehículo
- Recibe telemetría automática

**OBSERVER (Observador):**
- Solo comando `STATUS`
- No puede modificar estado
- Recibe telemetría automática

### 4.5 Manejo de Desconexiones

#### 4.5.1 Desconexión Normal

1. Cliente cierra conexión TCP
2. Servidor detecta cierre de socket
3. Limpia recursos del cliente
4. Registra evento en logs

#### 4.5.2 Desconexión Abrupta

1. Servidor detecta error en send/recv
2. Marca cliente como inactivo
3. Limpia recursos automáticamente
4. Registra evento de desconexión forzada

---

## 5. Seguridad del Protocolo

### 5.1 Autenticación Segura

#### 5.1.1 Hash de Contraseñas

- **Algoritmo**: SHA-256
- **Salt**: 64 caracteres hexadecimales únicos por usuario
- **Proceso**: `hash = SHA256(salt + password)`
- **Almacenamiento**: Solo hash y salt, nunca contraseña plana

#### 5.1.2 Generación de Salt

```python
import secrets
salt = secrets.token_hex(32)  # 64 caracteres hex
```

### 5.2 Protección contra Ataques

#### 5.2.1 Fuerza Bruta

- **Límite de Intentos**: 3 intentos por cuenta
- **Bloqueo Temporal**: 5 minutos (300 segundos)
- **Logging**: Todos los intentos se registran
- **Rate Limiting**: Por IP y por cuenta

#### 5.2.2 Inyección de Comandos

- **Validación Estricta**: Solo comandos predefinidos
- **Sanitización**: Limpieza de caracteres especiales
- **Longitud Máxima**: Límites en todos los campos
- **Parsing Seguro**: Uso de sscanf con límites

### 5.3 Logging y Auditoría

#### 5.3.1 Eventos Registrados

**Conexiones:**
```
[TIMESTAMP] CONNECTION [CONNECT] [unknown:NONE] from IP:PORT
[TIMESTAMP] CONNECTION [LOGIN_SUCCESS] [user:ROLE] from IP:PORT
[TIMESTAMP] CONNECTION [LOGIN_FAILED] [user:ROLE] from IP:PORT
[TIMESTAMP] CONNECTION [DISCONNECT] [user:ROLE] from IP:PORT
```

**Acciones de Usuario:**
```
[TIMESTAMP] USER_ACTION [user:ROLE] COMMAND: description
```

**Telemetría:**
```
[TIMESTAMP] Broadcast DATA: DATA values
```

---

## 6. Estados del Vehículo

### 6.1 Variables de Estado

#### 6.1.1 Velocidad
- **Tipo**: float
- **Rango**: 0.0 - ∞
- **Cálculo**: `velocidad_base + speed_offset`
- **Modificable**: Sí (ADMIN con SPEED UP)

#### 6.1.2 Dirección
- **Tipo**: enum (0=FORWARD, 1=LEFT, 2=RIGHT, 3=STOP)
- **Valores**: "FORWARD", "LEFT", "RIGHT", "STOP"
- **Modificable**: Sí (ADMIN con TURN LEFT/RIGHT)

#### 6.1.3 Batería
- **Tipo**: int
- **Rango**: 0-100%
- **Comportamiento**: Disminuye 5% cada 10s, auto-reset en 0
- **Modificable**: Sí (ADMIN con BATTERY RESET)

#### 6.1.4 Temperatura
- **Tipo**: int
- **Rango**: 20-40°C
- **Comportamiento**: Valor aleatorio simulado
- **Modificable**: No (solo lectura)

### 6.2 Transiciones de Estado

#### 6.2.1 Cambios de Dirección
```
FORWARD → LEFT    (TURN LEFT)
FORWARD → RIGHT   (TURN RIGHT)
LEFT → FORWARD    (automático después de tiempo)
RIGHT → FORWARD   (automático después de tiempo)
```

#### 6.2.2 Gestión de Batería
```
100% → 95% → 90% → ... → 5% → 0% → 100% (ciclo)
BATTERY RESET: cualquier_valor → 100%
```

---

## 7. Códigos de Error y Manejo de Excepciones

### 7.1 Códigos de Respuesta

| Código | Mensaje | Descripción |
|--------|---------|-------------|
| OK | `OK\n` | Comando ejecutado exitosamente |
| ERR_LOCKED | `ERR locked\n` | Cuenta bloqueada por intentos fallidos |
| ERR_INVALID | `ERR invalid (X/3)\n` | Credenciales inválidas |
| ERR_OBSERVER | `ERR observer_only\n` | Observer intentó comando de ADMIN |
| ERR_UNKNOWN | `ERR unknown_cmd\n` | Comando no reconocido |

### 7.2 Manejo de Errores de Red

#### 7.2.1 Errores de Conexión
- **ECONNRESET**: Cliente desconectado abruptamente
- **EPIPE**: Intento de escritura en socket cerrado
- **ETIMEDOUT**: Timeout de conexión

#### 7.2.2 Recuperación de Errores
- **Reconexión Automática**: No implementada (responsabilidad del cliente)
- **Limpieza de Recursos**: Automática en el servidor
- **Estado Persistente**: No se mantiene entre reconexiones

---

## 8. Ejemplos de Sesiones Completas

### 8.1 Sesión de Administrador Exitosa

```
Cliente → Servidor: LOGIN ADMIN root password\n
Servidor → Cliente: OK\n

Servidor → Cliente: DATA 45.2 100 FORWARD 25\n  (broadcast automático)

Cliente → Servidor: SPEED UP\n
Servidor → Cliente: OK\n

Cliente → Servidor: TURN LEFT\n
Servidor → Cliente: OK\n

Servidor → Cliente: DATA 46.2 95 LEFT 27\n  (broadcast automático)

Cliente → Servidor: STATUS\n
Servidor → Cliente: STATUS 46.2 95 LEFT 27\n

Cliente → Servidor: BATTERY RESET\n
Servidor → Cliente: OK\n

[Cliente cierra conexión]
```

### 8.2 Sesión de Observer

```
Cliente → Servidor: LOGIN OBSERVER juan 12345\n
Servidor → Cliente: OK\n

Servidor → Cliente: DATA 75.5 85 FORWARD 28\n  (broadcast automático)

Cliente → Servidor: STATUS\n
Servidor → Cliente: STATUS 75.5 85 FORWARD 28\n

Cliente → Servidor: SPEED UP\n
Servidor → Cliente: ERR observer_only\n

Servidor → Cliente: DATA 75.5 80 FORWARD 30\n  (broadcast automático)

[Cliente cierra conexión]
```

### 8.3 Sesión con Errores de Autenticación

```
Cliente → Servidor: LOGIN ADMIN root wrongpass\n
Servidor → Cliente: ERR invalid (1/3)\n

Cliente → Servidor: LOGIN ADMIN root badpass\n
Servidor → Cliente: ERR invalid (2/3)\n

Cliente → Servidor: LOGIN ADMIN root incorrect\n
Servidor → Cliente: ERR invalid (3/3)\n

Cliente → Servidor: LOGIN ADMIN root password\n
Servidor → Cliente: ERR locked\n

[Cliente debe esperar 5 minutos o reconectarse]
```

---

## 9. Consideraciones de Implementación

### 9.1 Concurrencia

#### 9.1.1 Threading Model
- **Hilo Principal**: Acepta conexiones entrantes
- **Hilo de Telemetría**: Genera y envía broadcasts cada 10s
- **Hilos de Cliente**: Un hilo por cliente conectado (hasta 100)

#### 9.1.2 Sincronización
- **Mutex**: Protege lista de clientes y variables globales
- **Thread-Safe**: Todas las operaciones críticas protegidas
- **Deadlock Prevention**: Orden consistente de adquisición de locks

### 9.2 Gestión de Memoria

#### 9.2.1 Estructuras de Datos
```c
struct client_data {
    int socket_fd;
    struct sockaddr_in addr;
    int authenticated;
    char role[16];
    char username[32];
    int active;
    int failed_attempts;
    int locked;
    time_t locked_until;
};
```

#### 9.2.2 Limpieza de Recursos
- **Automática**: Al detectar desconexión
- **Timeout**: Conexiones inactivas
- **Graceful Shutdown**: Señal SIGINT limpia todos los recursos

### 9.3 Configuración

#### 9.3.1 Archivo de Configuración (config.json)
```json
{
  "users": [
    {
      "role": "ADMIN",
      "username": "root",
      "salt": "c20efd8d3a1bbeb7a6645513d5b51b392974ba538b941c27bef5daaab2af5aff",
      "hash": "f0c6b39b2414f78e271b2933ca97aa782633e1108de23b935f84398ed9967972"
    }
  ]
}
```

#### 9.3.2 Parámetros de Servidor
- **Puerto**: Especificado como argumento de línea de comandos
- **Archivo de Log**: Especificado como argumento
- **Límites**: Definidos como constantes en código

---

## 10. Extensiones Futuras

### 10.1 Mejoras de Protocolo

#### 10.1.1 Versionado
- **Header de Versión**: `AVTS/1.0` en handshake inicial
- **Compatibilidad**: Soporte para múltiples versiones
- **Negociación**: Cliente y servidor acuerdan versión

#### 10.1.2 Compresión
- **Algoritmos**: gzip, lz4 para mensajes grandes
- **Negociación**: Opcional basada en capacidades
- **Overhead**: Evaluación de beneficio vs costo

### 10.2 Seguridad Avanzada

#### 10.2.1 Cifrado
- **TLS/SSL**: Cifrado de canal completo
- **Certificados**: Validación de identidad del servidor
- **Perfect Forward Secrecy**: Rotación de claves de sesión

#### 10.2.2 Autenticación Avanzada
- **2FA**: Autenticación de dos factores
- **Tokens JWT**: Tokens de sesión con expiración
- **OAuth2**: Integración con proveedores externos

### 10.3 Funcionalidades Adicionales

#### 10.3.1 Comandos Extendidos
- **EMERGENCY STOP**: Parada de emergencia
- **SET SPEED**: Establecer velocidad específica
- **GET LOGS**: Obtener logs del servidor
- **DIAGNOSTICS**: Información de diagnóstico

#### 10.3.2 Telemetría Avanzada
- **Frecuencia Variable**: Configurar intervalo de broadcast
- **Filtros**: Suscripción selectiva a tipos de datos
- **Histórico**: Almacenamiento y consulta de datos pasados

---

## 11. Conclusiones

El protocolo AVTS proporciona una base sólida para la comunicación entre clientes y el sistema de telemetría del vehículo autónomo. Sus características principales incluyen:

### 11.1 Fortalezas del Diseño

- **Simplicidad**: Protocolo basado en texto fácil de implementar y debuggear
- **Seguridad**: Autenticación robusta con protección contra ataques comunes
- **Escalabilidad**: Soporte para múltiples clientes concurrentes
- **Flexibilidad**: Sistema de roles permite diferentes niveles de acceso
- **Robustez**: Manejo comprehensivo de errores y desconexiones

### 11.2 Casos de Uso Cubiertos

- **Monitoreo en Tiempo Real**: Observadores pueden seguir el estado del vehículo
- **Control Remoto**: Administradores pueden modificar comportamiento del vehículo
- **Auditoría**: Sistema completo de logging para trazabilidad
- **Seguridad Operacional**: Controles de acceso y protección contra uso indebido

### 11.3 Consideraciones de Producción

Para un despliegue en producción, se recomendarían las siguientes mejoras:

- **Cifrado TLS**: Protección de datos en tránsito
- **Base de Datos**: Almacenamiento persistente de usuarios y logs
- **Load Balancing**: Distribución de carga entre múltiples servidores
- **Monitoring**: Métricas y alertas de sistema
- **Backup**: Respaldo de configuración y datos críticos

El protocolo AVTS cumple efectivamente con los objetivos de proporcionar una interfaz segura y eficiente para el control y monitoreo de vehículos autónomos en un entorno de simulación académica.


