// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DIALECT_SQLITE__INCLUDED
#define YB__ORM__DIALECT_SQLITE__INCLUDED

#include "orm/sql_driver.h"

namespace Yb {

class YBORM_DECL SQLite3Dialect: public SqlDialect
{
public:
    SQLite3Dialect();
    virtual bool native_driver_eats_slash();
    virtual const String select_curr_value(const String &seq_name);
    virtual const String select_next_value(const String &seq_name);
    virtual const String select_last_inserted_id(const String &table_name);
    virtual const String sql_value(const Value &x);
    virtual const String type2sql(int t);
    virtual bool fk_internal();
    virtual bool has_for_update();
    virtual const String create_sequence(const String &seq_name);
    virtual const String drop_sequence(const String &seq_name);
    virtual const String primary_key_flag();
    virtual const String autoinc_flag();
    // schema introspection
    virtual bool table_exists(SqlConnection &conn, const String &table);
    virtual bool view_exists(SqlConnection &conn, const String &table);
    virtual Strings get_tables(SqlConnection &conn);
    virtual Strings get_views(SqlConnection &conn);
    virtual ColumnsInfo get_columns(SqlConnection &conn, const String &table);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DIALECT_SQLITE__INCLUDED
