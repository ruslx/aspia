//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "router/manager/router_window_proxy.h"

#include "base/logging.h"
#include "base/task_runner.h"
#include "proto/router_admin.pb.h"
#include "router/manager/router_window.h"

namespace router {

RouterWindowProxy::RouterWindowProxy(std::shared_ptr<base::TaskRunner> ui_task_runner,
                                     RouterWindow* router_window)
    : ui_task_runner_(std::move(ui_task_runner)),
      router_window_(router_window)
{
    DCHECK(ui_task_runner_ && router_window_);
}

RouterWindowProxy::~RouterWindowProxy()
{
    DCHECK(!router_window_);
}

void RouterWindowProxy::dettach()
{
    DCHECK(ui_task_runner_->belongsToCurrentThread());
    router_window_ = nullptr;
}

void RouterWindowProxy::onConnected(const base::Version& peer_version)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onConnected, shared_from_this(), peer_version));
        return;
    }

    if (router_window_)
        router_window_->onConnected(peer_version);
}

void RouterWindowProxy::onDisconnected(base::NetworkChannel::ErrorCode error_code)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onDisconnected, shared_from_this(), error_code));
        return;
    }

    if (router_window_)
        router_window_->onDisconnected(error_code);
}

void RouterWindowProxy::onAccessDenied(base::ClientAuthenticator::ErrorCode error_code)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onAccessDenied, shared_from_this(), error_code));
        return;
    }

    if (router_window_)
        router_window_->onAccessDenied(error_code);
}

void RouterWindowProxy::onHostList(std::shared_ptr<proto::HostList> host_list)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onHostList, shared_from_this(), host_list));
        return;
    }

    if (router_window_)
        router_window_->onHostList(host_list);
}

void RouterWindowProxy::onHostResult(std::shared_ptr<proto::HostResult> host_result)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onHostResult, shared_from_this(), host_result));
        return;
    }

    if (router_window_)
        router_window_->onHostResult(host_result);
}

void RouterWindowProxy::onRelayList(std::shared_ptr<proto::RelayList> relay_list)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onRelayList, shared_from_this(), relay_list));
        return;
    }

    if (router_window_)
        router_window_->onRelayList(relay_list);
}

void RouterWindowProxy::onUserList(std::shared_ptr<proto::UserList> user_list)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onUserList, shared_from_this(), user_list));
        return;
    }

    if (router_window_)
        router_window_->onUserList(user_list);
}

void RouterWindowProxy::onUserResult(std::shared_ptr<proto::UserResult> user_result)
{
    if (!ui_task_runner_->belongsToCurrentThread())
    {
        ui_task_runner_->postTask(
            std::bind(&RouterWindowProxy::onUserResult, shared_from_this(), user_result));
        return;
    }

    if (router_window_)
        router_window_->onUserResult(user_result);
}

} // namespace router
