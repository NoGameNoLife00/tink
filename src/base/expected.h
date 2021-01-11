#ifndef TINK_EXPECTED_H
#define TINK_EXPECTED_H

#include <string>
#include <optional>

namespace tink {
    template <typename T>
    using optional = std::optional<T>;
    enum class ErrorCodes {
        ERR_OK = 0,
    };

    class Status {
    public:
        Status();
        Status(const ErrorCodes& code, const std::string& reason);
        Status(const Status& st) = default;
        Status(Status&& st);
        Status& operator=(const Status& st) = default;
        ~Status();
        bool OK() const;
        std::string ToString() const;
        ErrorCodes Code() const;
        const std::string& GetErrmsg() const;

        static std::string GetErrString(ErrorCodes code);

    private:
        std::string errmsg_;
        ErrorCodes code_;
    };

    // https://meetingcpp.com/2017/talks/items/Introduction_to_proposed_std__expected_T__E_.html
    template <typename T>
    class Expected {
    public:
        static_assert(!std::is_same<T, Status>::value, "invalid use of recursive Expected<Status>");

        Expected(ErrorCodes code, const std::string& reason)
        : status_(Status(code, reason)) {}

        Expected(const Status& st) : status_(st) {}

        Expected(const T& t)
        : data_(t), status_(Status(ErrorCodes::ERR_OK, "")) {}

        Expected(T&& t)
        : data_(std::move(t)), status_(Status(ErrorCodes::ERR_OK, "")) {}

        const T& GetValue() const {
            return *data_;
        }

        T& GetValue() {
            return *data_;
        }

        const Status& GetStatus() const {
            return status_;
        }

        bool OK() const {
            return status_.OK();
        }

    private:
        optional<T> data_;
        Status status_;
    };



};


#endif //TINK_EXPECTED_H
