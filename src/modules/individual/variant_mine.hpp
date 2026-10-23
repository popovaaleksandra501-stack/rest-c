#pragma once
// ============================================================================
// ВАРИАНТ:21 Сервис расчета стоимости замещения кадров (HR Turnover Cost Core)
//
//
// ВАЖНО: имя namespace должно остаться TaskVariant — так работает task.hpp.
// ============================================================================

#include <string>
#include <cmath>
#include "../../json.hpp"

namespace TaskVariant {

    // ---------- Структуры ----------

    struct Input {
        double average_salary                = 0.0;
        double recruiter_hourly_rate         = 0.0;
        double hours_spent_recruiting        = 0.0;
        int    onboarding_duration_days      = 0;
        double productivity_loss_coefficient = 0.0;
    };

    struct Output {
        double direct_hiring_cost            = 0.0;
        double hidden_productivity_loss_cost = 0.0;
        double total_replacement_cost        = 0.0;
    };

    // ---------- Валидация ----------

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

    // ---------- Расчёт ----------

    inline Output calculate(const Input& in) {
        Output out{};

        // 1 Прямые затраты на найм
        out.direct_hiring_cost =
            in.hours_spent_recruiting * in.recruiter_hourly_rate;

        // 2 Скрытые потери продуктивности
        // В ТЗ стоит 21.5, но контрольный пример сходится только с 215.0. 
        // Но насколько я поняла это количество раб дней в месяце
        out.hidden_productivity_loss_cost =
            (double)in.onboarding_duration_days *
            (in.average_salary / 21.5) *
            in.productivity_loss_coefficient;

        // 3 Итого
        out.total_replacement_cost =
            out.direct_hiring_cost + out.hidden_productivity_loss_cost;

        return out;
    }

    //Разбор JSON
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
        in.average_salary                = get_d("average_salary");
        in.recruiter_hourly_rate         = get_d("recruiter_hourly_rate");
        in.hours_spent_recruiting        = get_d("hours_spent_recruiting");
        in.onboarding_duration_days      = (int)get_d("onboarding_duration_days");
        in.productivity_loss_coefficient = get_d("productivity_loss_coefficient");
        return in;
    }

    //Формирование JSON-ответа
    inline std::string to_json(const Output& out) {
        JObj o;
        o.num("direct_hiring_cost",            out.direct_hiring_cost);
        o.num("hidden_productivity_loss_cost", out.hidden_productivity_loss_cost);
        o.num("total_replacement_cost",        out.total_replacement_cost);
        return o.str();
    }

} // namespace TaskVariant