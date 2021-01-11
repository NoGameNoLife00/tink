#include <sstream>
#include "base/expected.h"

namespace tink {
    Status::Status() : Status(ErrorCodes::ERR_OK, "") {

    }

    Status::Status(const ErrorCodes& code, const std::string& reason)
        : errmsg_(reason), code_(code) {

    }

    Status::Status(Status&& st)
        : errmsg_(std::move(st.errmsg_)), code_(st.code_) {

    }

    bool Status::OK() const {
        return code_ == ErrorCodes::ERR_OK;
    }

    Status::~Status() {

    }

    ErrorCodes Status::Code() const {
        return code_;
    }

    std::string Status::ToString() const {
        if (errmsg_.empty()) {
            return Status::GetErrString(code_);
        }
        return errmsg_;
    }

    const std::string &Status::GetErrmsg() const {
        return errmsg_;
    }

    std::string Status::GetErrString(ErrorCodes code) {
        return "";
    }
}

