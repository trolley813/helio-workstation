/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "HelioApiRoutes.h"
#include "BackendRequest.h"
#include "Config.h"
#include "SerializationKeys.h"

class TokenUpdateThread final : public Thread
{
public:
    
    TokenUpdateThread() : Thread("TokenUpdate") {}
    ~TokenUpdateThread() override
    {
        this->stopThread(1000);
    }

    Function<void(const String &newToken)> onTokenUpdateOk;
    Function<void(const Array<String> &errors)> onTokenUpdateFailed;

    void updateToken(String lastValidToken, uint32 delayMs)
    {
        if (this->isThreadRunning())
        {
            return;
        }

        this->delay = delayMs;
        this->oldToken = lastValidToken;
        this->startThread(3);
    }
    
private:
    
    void run() override
    {
        namespace ApiKeys = Serialization::Api::V1;
        namespace ApiRoutes = Routes::HelioFM::Api;

        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + this->delay);

        // Construct payload object:
        ValueTree session(ApiKeys::session);
        session.setProperty(ApiKeys::bearer, this->oldToken, nullptr);
        session.setProperty(ApiKeys::deviceId, Config::getDeviceId(), nullptr);
        session.setProperty(ApiKeys::platformId, SystemStats::getOperatingSystemName(), nullptr);

        const BackendRequest request(ApiRoutes::tokenUpdate);
        this->response = request.post(session);

        if (!this->response.isValid() ||
            !this->response.is2xx() ||
            !this->response.hasProperty(ApiKeys::token))
        {
            callbackOnMessageThread(TokenUpdateThread, onTokenUpdateFailed, self->response.getErrors());
            return;
        }

        callbackOnMessageThread(TokenUpdateThread, onTokenUpdateOk,
            self->response.getProperty(ApiKeys::token));
    }
    
    uint32 delay;
    String oldToken;
    BackendRequest::Response response;
    
    friend class BackendService;
};
