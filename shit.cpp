#include <iostream>
#include <postgresql/libpq-fe.h>

int db_query() {
    // Параметры подключения
    const char* conninfo = "dbname=linuxgram user=ilya hostaddr=127.0.0.1 port=5432 password=qwerty";

    // Установка соединения с базой данных
    PGconn* conn = PQconnectdb(conninfo);

    // Проверка состояния подключения
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Ошибка подключения к базе данных: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return 1;
    }

    // Выполнение SQL-запроса
    PGresult* res = PQexec(conn, "SELECT version();");

    // Проверка результата выполнения запроса
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        PQfinish(conn);
        return 1;
    }

    // Вывод результата
    std::cout << "Версия PostgreSQL: " << PQgetvalue(res, 0, 0) << std::endl;

    // Освобождение ресурсов
    PQclear(res);
    PQfinish(conn);

    return 0;
}