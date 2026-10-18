Выполненый 21 вариант задания. Очет прикреплен в сдо и продублирован в read.me

Попова Александра Отчет
Вариант 21. Сервис расчета стоимости замещения кадров (HR Turnover Cost Core)
Вход: average_salary (double, >=500.0), recruiter_hourly_rate (double, >0.0), hours_spent_recruiting (double, [1.0, 500.0]), onboarding_duration_days (int, [1, 180]), productivity_loss_coefficient (double, [0.05, 1.0])
Формулы:
	direct_hiring_cost = hours_spent_recruiting recruiter_hourly_rate
	hidden_productivity_loss_cost = onboarding_duration_days (average_salary / 21.5) productivity_loss_coefficient
	total_replacement_cost = direct_hiring_cost + hidden_productivity_loss_cost

Входные данные (чтобы не искать в отчете) 
{
    "average_salary": 60000.0,
    "recruiter_hourly_rate": 50.0,
    "hours_spent_recruiting": 40.0,
    "onboarding_duration_days": 30,
    "productivity_loss_coefficient": 0.3
  }
  
ГЛАВА 1. ИССЛЕДОВАНИЕ ВХОДНОГО ПРОЕКТА
1.1. Основы REST-API
REST (Representational State Transfer) — архитектурный стиль взаимодействия компонентов распределённого приложения в сети. REST-API представляет собой набор HTTP-эндпоинтов, каждый из которых соответствует определённому ресурсу и поддерживает стандартные методы HTTP: GET (получение), POST (создание), PUT (обновление), DELETE (удаление).
Основные принципы REST, реализованные в рассматриваемом проекте:
•	Клиент-серверная архитектура — клиент (веб-интерфейс или curl) и сервер взаимодействуют посредством HTTP-запросов
•	Отсутствие состояния (stateless) — каждый запрос содержит всю необходимую информацию, аутентификация выполняется через токен (Bearer Token)
•	Кэшируемость — ответы сервера содержат заголовки, позволяющие кэшировать данные
•	Единообразие интерфейса — все эндпоинты используют единый формат JSON для передачи данных
•	Слоистая система — маршрутизация, бизнес-логика и работа с БД разделены по модулям

1.2. Структура проекта и назначение модулей
Проект организован по модульному принципу. Основные компоненты:
REST-API/
├── CMakeLists.txt                          Сборка
├── main.cpp                                Точка входа + middleware
├── README.md                               Этот файл
└── src/
    ├── json.hpp                            Сборка JSON-ответов
    ├── crypto.hpp                          SHA-256, пароли, токены
    ├── database.hpp                        Работа с SQLite + конфиг
    │
    ├── routes/
    │   ├── auth.hpp                        8 эндпоинтов авторизации
    │   └── public.hpp                      Счётчики, история, admin-эндпоинты
    │
    ├── modules/
    │   ├── example/
    │   │   └── stats.hpp                   Демо: статистика (как пример)
    │   └── individual/
    │       ├── task.hpp                    Каркас индивидуального задания (не трогать)
    │       ├── variant_mine.hpp Сервис расчета стоимости замещения кадров
    │       ├── variant_roi.hpp             Пример: ROMI/ROAS
    │       └── README.md                   ТЗ-Инструкция к выполнению
    │
    └── lib/
        ├── httplib.h                       Сторонний HTTP-движок
        └── sqlite_loader.hpp               Загрузчик SQLite

1.3. Сборка и запуск проекта
Проект собирается с использованием CMake (версия 3.10 и выше). Типовая последовательность команд:
cd C:\user\rest-main-c (на диск c закинула)
Remove-Item -Recurse -Force CMakeCache.txt, CMakeFiles, build.ninja, .ninja_deps, .ninja_log, cmake_install.cmake -ErrorAction SilentlyContinue
cmake .
cmake --build .
.\rest_server.exe
Сервер запускается на порту 8080

1.4. Схема базы данных и роли пользователей
(из database.hpp). Четыре таблицы: users (хранит учётные записи пользователей), sessions (Хранит активные сессии пользователей), works (Хранит историю выполненных расчётов индивидуального модуля), logs (Системный журнал действий и событий)
Таблица users
Поле	Тип	Ограничения	Описание
Id	Int Primary key 	Primary key autoincrement	Идентификатор пользователя
username	text	unique	Логин пользователя
password	text		Хеш пароля
role	text	defult 'user'		Роль: user, admin, owner
Пароли в открытом виде не хранятся — при инициализации и регистрации выполняется хеширование. Поле username объявлено как UNIQUE, что гарантирует уникальность логинов на уровне СУБД и приводит к ошибке 409 Conflict при попытке регистрации дубликата.
Таблица sessions
Поле	Тип	Ограничения	Описание
token	text	Primary key	Токен доступа (Bearer Token)
user_id	int	Foreign key	Ссылка на users.id
expires_at	text		Время истечения токена в формате "YYYY-MM-DD HH:MM:SS"

Время жизни токена задаётся константой TOKEN_TTL_SECONDS = 300 (5 минут). При истечении токена эндпоинты, требующие авторизации, возвращают 401 Unauthorized. Обновление токена выполняется через эндпоинт /api/v1.1/auth/refresh. Почему-то дата хранится в формате текста….
Таблица works
Поле	Тип	Ограничения	Описание
Id	Int Primary key 	Primary key autoincrement	Идентификатор записи
user_id	int	Foreign key	Ссылка на users.id
datetime	text		Дата и время выполнения (now_iso())
input_json	text		Входные данные в формате JSON
result_json	text		Результат расчёта в формате JSON

Именно в эту таблицу сохраняется результат успешного вызова /api/v1.1/works/calc-individual. История доступна пользователю через GET /api/v1.1/works/history-individual, при этом выборка ограничивается записями текущего пользователя (WHERE user_id = ?) и лимитом MAX_HISTORY_LIMIT = 20 записей
Таблица logs
Поле	Тип	Ограничения	Описание
Id	Int Primary key 	Primary key autoincrement	Идентификатор записи
datetime	text		Время события
status	text		Статус/категория события
message	text		Текстовое описание события

Запись в лог выполняется функцией write_log(status, message) из database.hpp. Просмотр логов доступен только ролям admin и owner через административные эндпоинты.
Роли пользователей
В системе предусмотрено три роли, определяющие уровень доступа:
Роль	Описание	Права
user	Обычный пользователь	Расчёты, своя история, смена своего пароля
admin		Администратор	Всё, что user, плюс управление пользователями (кроме owner), просмотр логов и статистики, запуск автотестов

owner	Владелец	Полные права, включая права admin; удаление учётной записи owner1 запрещено
Стартовые пользователи
При инициализации БД создаются три учётные записи:
Логин	Пароль	Роль
owner1	owner1	owner
admin1	admin1	admin
user1	user1	user

1.5. Обзор эндпоинтов и их назначение
Гостевые эндпоинты (без токена):
Метод	Путь	Назначение
GET	/health	Проверка работоспособности сервера
GET	/api/v1.1/counters	Публичная статистика
GET	/api/v1.1/history	Публичная история
POST	/api/v1.1/auth/register	Регистрация
POST	/api/v1.1/auth/login	Вход, выдача токена
Пользовательские эндпоинты (токен обязателен):
Метод	Путь	Назначение
GET	/api/v1.1/auth/profile	Профиль текущего пользователя
POST	/api/v1.1/auth/refresh	Обновление токена
POST	/api/v1.1/auth/logout	Выход
PUT	/api/v1.1/auth/users/password	Смена своего пароля
POST	/api/v1.1/works/calc	Демонстрационный расчёт (ROI)
GET	/api/v1.1/works/history	История демонстрационных расчётов
POST	/api/v1.1/works/calc-individual	Расчёт по индивидуальному варианту
GET	/api/v1.1/works/history-individual	История индивидуальных расчётов
Административные эндпоинты:
Метод	Путь	Назначение
GET	/api/v1.1/auth/users	Список пользователей
PUT	/api/v1.1/auth/users/password	Смена чужого пароля
DELETE	/api/v1.1/auth/users	Удаление пользователя
GET	/api/v1.1/admin/stats	Статистика системы
GET	/api/v1.1/admin/run-tests	Запуск встроенных автотестов

Коды ответов:
Код	Ситуация
200 / 201	Успех
400	Некорректные данные
401	Нет токена / просрочен
403	Недостаточно прав
404	Не найдено
409	Конфликт (дубль)
500	Ошибка сервера

1.6. Описание инструментов тестирования через web и curl
Для тестирования эндпоинтов используются два основных инструмента.
Веб-интерфейс (frontend/index.html) предоставляет графическую оболочку: поля для ввода метода и URL, области для JSON-тела и заголовков, кнопку отправки запроса, а также область отображения ответа сервера (статус, время отклика, тело) и историю запросов
curl — консольная утилита для отправки HTTP-запросов
1.7. Проверка корректности работы эндпоинтов
В ходе исследования были проверены все 13 эндпоинтов. Ключевые результаты:
•	/health возвращает 200 OK с информацией о состоянии сервера.
•	/api/v1.1/auth/login при корректных данных возвращает токен; при неверном пароле 401.
•	/api/v1.1/auth/profile без токена возвращает 401 Unauthorized; с токеном данные пользователя.
•	/api/v1.1/works/calc-individual расчёт выполняется успешно, результат сохраняется в БД
•	/api/v1.1/works/history возвращает историю расчётов пользователя.
•	Административные эндпоинты при обращении от имени user1 возвращают 403 Forbidden
•	Регистрация позволяет зарегистрироваться ролью admin, но, к счастью, всё равно присваевает роль user (администрирование пользователю недоступно, все отлично)
 
 
Все проверки подтвердили работоспособность серверной части и корректность разграничения прав доступа. 
1.8. Описание выявленных несоответствий и ошибок
В ходе тестирования были выявлены следующие особенности:
1.	При обращении к эндпоинту DELETE /api/v1.1/auth/users администратор (admin1) может указать в теле запроса собственный user_id и успешно удалить себя. Сервер при этом не проверяет, что удаляемый пользователь не совпадает с текущим авторизованным пользователем. Надо бы перед удалением проверять, что target_user_id != current_user_id, и возвращать 400 Bad Request или 403 Forbidden с пояснением, что удаление собственной учётной записи через административный эндпоинт не допускается.
 
2.	Смена чужого пароля (PUT /api/v1.1/auth/users/password) разрешена роли admin. При этом отдельного запрета на изменение пароля owner в коде нет (только касательно удаления owner1). А значит администратор может сменить пароль владельцу и фактически лишить владельца доступа. Нарушена иерархия ролей. Роль owner заявлена как обладающая «полными правами, включая admin», но фактически администратор может нейтрализовать владельца, не удаляя его учётную запись. Необходимо добавить проверку, запрещающую роли admin изменять пароль пользователей с ролью owner. Такая операция должна быть доступна только самому владельцу (смена своего пароля) либо другому владельцу.
 
3.	В коде нет ограничений на размер json. Сервер не ограничивает размер входящего JSON-тела. Злоумышленник может отправить запрос с телом в десятки мегабайт
4.	Эндпоинт POST /api/v1.1/auth/login не ограничивает количество попыток. Перебор пароля к admin1/owner1 возможен без блокировки. . Можно ввести счётчик неудачных попыток по username и/или IP, временно блокировать после N попыток (например, 5) на M минут, логировать события в logs.
5.	Почти все поля в бд хранятся как текст, даже поля дат и айди. Насколько я поняла, для sqllite это некритично, но если перейти на postgres он будет ругаться
1.9. Запуск встроенных автотестов
Для запуска встроенных автотестов используется эндпоинт GET /api/v1.1/admin/run-tests, доступный ролям admin и owner. Блок автотестов включает 20 проверок, покрывающих:
•	корректные данные (несколько наборов);
•	граничные значения (минимумы и максимумы ограничений);
•	неполные данные (отсутствие полей);
•	неверные типы данных;
•	нарушение ограничений (отрицательные значения, превышение максимумов);
•	проверку авторизации (без токена, с недействительным токеном).
 
 
ГЛАВА 2. РАЗРАБОТКА ЛОГИКИ МОДУЛЯ
2.1. Постановка задачи
Вариант 21. Сервис расчета стоимости замещения кадров (HR Turnover Cost Core)
Вход: average_salary (double, >=500.0), recruiter_hourly_rate (double, >0.0), hours_spent_recruiting (double, [1.0, 500.0]), onboarding_duration_days (int, [1, 180]), productivity_loss_coefficient (double, [0.05, 1.0])
Формулы:
direct_hiring_cost = hours_spent_recruiting × recruiter_hourly_rate
hidden_productivity_loss_cost = onboarding_duration_days × (average_salary / 21.5) × productivity_loss_coefficient
total_replacement_cost = direct_hiring_cost + hidden_productivity_loss_cost

2.2. Словесные алгоритм
1.	Сервер принимает POST-запрос на /api/v1.1/works/calc-individual.
2.	Проверяется наличие и валидность токена авторизации. При отсутствии — возврат 401.
3.	Извлекается JSON-тело запроса.
4.	Вызывается разбор полей в структуру Input.
5.	Вызывается проверка всех ограничений.
6.	При нарушении ограничений возврат 400 Bad Request с описанием ошибки.
7.	Вызывается выполнение расчётов по формулам.
8.	Формирование JSON-ответа.
9.	Результат сохраняется в таблицу works (поля user_id, datetime, input_json, result_json).
10.	Возврат 200 OK с JSON-ответом.
2.3. Блок-схемы алгоритма
 
ГЛАВА 3. ПРОГРАММНАЯ РЕАЛИЗАЦИЯ
3.1. Выбор конструкций C++ и библиотек
При реализации модуля использовались следующие конструкции и средства C++17:
•	Структуры Input и Output — для агрегации входных и выходных данных. Все поля имеют инициализаторы по умолчанию (= 0.0 / = 0), что гарантирует нулевое состояние при создании Input in{}; и Output out{}; и исключает чтение неинициализированной памяти.
•	Пространство имён TaskVariant — обязательное требование задания; имя зафиксировано, поскольку task.hpp обращается к функциям именно через TaskVariant::.
•	Inline-функции (inline std::string validate(...), inline Output calculate(...) и т. д.) — размещение в заголовочном файле без нарушения правила одного определения (ODR) при подключении из нескольких единиц трансляции.
•	Лямбда-выражение get_d в parse() — для компактного извлечения числового поля по имени ключа.
•	Явное приведение (int) — для приведения onboarding_duration_days из double к int после извлечения через get_d.
•	Обёртка JObj из json.hpp — для формирования JSON-ответа: метод num(name, value) добавляет числовое поле, метод str() возвращает итоговую строку
3.2. Реализация модуля варианта
Ограничения:
Параметр	Тип	Ограничение	Описание
average_salary	double	>= 500.0	Средняя зарплата сотрудника
recruiter_hourly_rate	double	> 0.0	Почасовая ставка рекрутера
hours_spent_recruiting	double	[1.0, 500.0]	Часы на подбор
onboarding_duration_days	int	[1, 180]	Длительность адаптации (дни)
productivity_loss_coefficient	double	[0.05, 1.0]	Коэффициент потери эффективности
Структуры входных и выходных данных
Обе структуры используют инициализацию полей значениями по умолчанию (= 0.0, = 0). Если функция parse() не найдёт какое-либо поле в JSON-теле, соответствующее значение останется нулевым, и затем сработает валидация. Таким образом исключается неопределённое поведение из-за неинициализированных полей.
struct Input {
    double average_salary = 0.0;
    double recruiter_hourly_rate = 0.0;
    double hours_spent_recruiting = 0.0;
    int    onboarding_duration_days = 0;
    double productivity_loss_coefficient = 0.0;
};
Возвращает:
struct Output {
    double direct_hiring_cost = 0.0;
    double hidden_productivity_loss_cost = 0.0;
    double total_replacement_cost = 0.0;
};

Валидация
Функция validate() последовательно проверяет все пять ограничений варианта. При первом же нарушении возвращается строка с описанием ошибки
inline std::string validate(const Input& in) {
    if (in.average_salary < 500.0)
        return "average_salary must be >= 500.0";
    if (in.recruiter_hourly_rate <= 0.0)
        return "recruiter_hourly_rate must be > 0.0";
    if (in.hours_spent_recruiting < 1.0 || in.hours_spent_recruiting > 500.0)
        return "hours_spent_recruiting must be in [1.0, 500.0]";
    if (in.onboarding_duration_days < 1 || in.onboarding_duration_days > 180)
        return "onboarding_duration_days must be in [1, 180]";
    if (in.productivity_loss_coefficient < 0.05 ||
        in.productivity_loss_coefficient > 1.0)
        return "productivity_loss_coefficient must be in [0.05, 1.0]";
    return "";
}
Расчёт
inline Output calculate(const Input& in) {
    Output out{};
    // 1. Прямые затраты на найм
    out.direct_hiring_cost =
        in.hours_spent_recruiting * in.recruiter_hourly_rate;
    // 2. Скрытые потери продуктивности
    out.hidden_productivity_loss_cost =
        (double)in.onboarding_duration_days *
        (in.average_salary / 21.5) *
        in.productivity_loss_coefficient;
    // 3. Итого
    out.total_replacement_cost =
        out.direct_hiring_cost + out.hidden_productivity_loss_cost;
    return out;
}
Разбор JSON:
inline Input parse(const std::string& body) {
    Input in{};
    auto get_d = [&](const std::string& key) -> double {
        auto p = body.find("\"" + key + "\"");
        if (p == std::string::npos) return 0.0;
        p = body.find(':', p);
        if (p == std::string::npos) return 0.0;
        try { return std::stod(body.substr(p + 1)); }
        catch (...) { return 0.0; }
    };
    in.average_salary  = get_d("average_salary");
    in.recruiter_hourly_rate = get_d("recruiter_hourly_rate");
    in.hours_spent_recruiting  = get_d("hours_spent_recruiting");
    in.onboarding_duration_days  = (int)get_d("onboarding_duration_days");
    in.productivity_loss_coefficient = get_d("productivity_loss_coefficient");
    return in;
}

Формирование JSON-ответа:
inline std::string to_json(const Output& out) {
    JObj o;
    o.num("direct_hiring_cost",            out.direct_hiring_cost);
    o.num("hidden_productivity_loss_cost", out.hidden_productivity_loss_cost);
    o.num("total_replacement_cost",        out.total_replacement_cost);
    return o.str();
}
Раздел 3.3. Встраивание модуля в систему
Модуль подключается в task.hpp одной директивой:
#include "variant_mine.hpp"
(Ранее в этой строке был #include "variant_roi.hpp"). 

ГЛАВА 4. ТЕСТИРОВАНИЕ И ПУБЛИКАЦИЯ
4.1. Методика тестирования
Тестирование проводилось по следующим направлениям:
1.	Контрольный пример —- проверка расчёта на данных из задания с ручной верификацией.
2.	Граничные значения — проверка поведения модуля на минимальных и максимальных допустимых значениях каждого параметра.
3.	Невалидные данные — проверка реакции на нарушение ограничений.
4.	Проверка авторизации — доступ без токена и с токеном.
5.	Встроенные автотесты — запуск блока из 20 проверок через административный эндпоинт.
Инструменты: веб-интерфейс frontend/index.html и утилита curl.
4.2. Контрольный пример
Входные данные (из ТЗ):
{
  "average_salary": 60000.0,
  "recruiter_hourly_rate": 50.0,
  "hours_spent_recruiting": 40.0,
  "onboarding_duration_days": 30,
  "productivity_loss_coefficient": 0.3
}
 
4.3. Граничные значения
Минимальные значения
Минимальная зп (удостоверимся что это вообще работает)
{
"average_salary":500.0,
"recruiter_hourly_rate":50.0,
"hours_spent_recruiting":40.0,
"onboarding_duration_days":30,
"productivity_loss_coefficient":0.3
}
 
Возвращает 200, так что сформируем json с минимальнымит значениями всех полей, для проверки всех ограничений
{
  "average_salary": 500.0,
  "recruiter_hourly_rate": 0.01,
  "hours_spent_recruiting": 1.0,
  "onboarding_duration_days": 1,
  "productivity_loss_coefficient": 0.05
}
 
Максимальные значения (у полей average_salary и recruiter_hourly_rate в ТЗ нет верхней границы)
{
  "average_salary": 60000,
  "recruiter_hourly_rate": 50,
  "hours_spent_recruiting": 500.0,
  "onboarding_duration_days": 180,
  "productivity_loss_coefficient": 1.0
}
 
4.4. Невалидные данные
Зп вне доспустимого 
{
  "average_salary": 40.0,
  "recruiter_hourly_rate": 50.0,
  "hours_spent_recruiting": 40.0,
  "onboarding_duration_days": 30,
  "productivity_loss_coefficient": 0.3
}
 
Сообщения при тестировании других недопустимых значений идентичны.
4.5. Проверка авторизации
Без токена: запрос к /api/v1.1/works/calc-individual возвращает ошибку 401
 
С токеном user1: запрос выполняется успешно, возвращается 200 OK с результатом расчёта. Результат сохраняется в таблицу works и доступен через GET /api/v1.1/works/history-individual
 

С токеном admin1 и owner1: также успешно, поскольку эндпоинт доступен всем авторизованным ролям
Неправильный пароль:
 

Получается, всё работает.
4.6. Публикация в git-репозиторий

ЗАКЛЮЧЕНИЕ
•	В результате выполнения работы:
•	Изучена архитектура готового REST-сервера и его эндпоинты.
•	Разработан модуль расчёта стоимости замещения кадров в соответствии с вариантом 21.
•	Модуль встроен в систему через task.hpp и успешно собирается.
•	Проведено тестирование: контрольный пример, граничные значения, невалидные данные, проверка авторизации.
•	Результаты совпадают с ручными расчётами.
•	Проект опубликован в git-репозитории.
