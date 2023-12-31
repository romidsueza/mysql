/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "sql_user_require.h"
#include "ngs_common/options.h"

using namespace xpl;

const std::string Sql_user_require::SSL_TYPE_NONE = "";
const std::string Sql_user_require::SSL_TYPE_SSL = "ANY";
const std::string Sql_user_require::SSL_TYPE_X509 = "X509";
const std::string Sql_user_require::SSL_TYPE_SPECIFIC = "SPECIFIED";

ngs::Error_code Sql_user_require::validate(ngs::IOptions_session_ptr &options) const
{
  if (ssl_type == SSL_TYPE_NONE)
    return ngs::Error_code();
  else if (ssl_type == SSL_TYPE_SSL)
    return check_ssl(options);
  else if (ssl_type == SSL_TYPE_X509)
    return check_x509(options);
  else if (ssl_type == SSL_TYPE_SPECIFIC)
    return check_specific(options);

  return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Unknown SSL required option.");
}

ngs::Error_code Sql_user_require::check_ssl(ngs::IOptions_session_ptr &options) const
{
  if (!options->active_tls())
    return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Current account requires TLS to be activate.");

  return ngs::Error_code();
}

ngs::Error_code Sql_user_require::check_x509(ngs::IOptions_session_ptr &options) const
{
  ngs::Error_code error;

  if ((error = check_ssl(options)))
    return error;

  if (options->ssl_get_verify_result_and_cert() != X509_V_OK)
    return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Current account requires TLS to be activate.");

  return ngs::Error_code();
}

ngs::Error_code Sql_user_require::check_specific(ngs::IOptions_session_ptr &options) const
{
  ngs::Error_code error;

  if ((error = check_x509(options)))
    return error;

  if (ssl_cipher.length())
  {
    if (ssl_cipher != options->ssl_cipher())
      return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Current user cipher isn't allowed.");
  }

  if (ssl_x509_issuer.length() &&  ssl_x509_issuer != options->ssl_get_peer_certificate_issuer())
    return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Current user certificate issuer is not valid.");

  if (ssl_x509_subject.length() &&  ssl_x509_subject != options->ssl_get_peer_certificate_subject())
    return ngs::Error_code(ER_SECURE_TRANSPORT_REQUIRED, "Current user certificate subject is not valid.");

  return ngs::Error_code();
}
