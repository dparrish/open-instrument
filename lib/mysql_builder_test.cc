/*
 *  -
 *
 * (c) 2010 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gtest/gtest.h>
#include "lib/common.h"
#include "lib/mysql_builder.h"

namespace openinstrument {

class MysqlBuilderTest : public ::testing::Test {};

TEST_F(MysqlBuilderTest, TableFromString) {
  string input = "CREATE TABLE `values` (\n"
                 "  `variable` bigint(20) NOT NULL,\n"
                 "  `when` bigint(20) NOT NULL,\n"
                 "  `value` double NOT NULL,\n"
                 "  UNIQUE KEY `variable` (`variable`,`value`)\n"
                 ") ENGINE=MyISAM DEFAULT CHARSET=utf8";

  string input2 = "CREATE TABLE IF NOT EXISTS `adminnotification_inbox` (\n"
                  "  `notification_id` int(10) unsigned NOT NULL AUTO_INCREMENT,\n"
                  "  `severity` tinyint(3) unsigned NOT NULL DEFAULT '0',\n"
                  "  `date_added` datetime NOT NULL,\n"
                  "  `title` varchar(255) NOT NULL,\n"
                  "  `description` text,\n"
                  "  `url` varchar(255) NOT NULL,\n"
                  "  `is_read` tinyint(1) unsigned NOT NULL DEFAULT '0',\n"
                  "  `is_remove` tinyint(1) unsigned NOT NULL DEFAULT '0',\n"
                  "  PRIMARY KEY (`notification_id`),\n"
                  "  KEY `IDX_SEVERITY` (`severity`,`description`),\n"
                  "  KEY (`is_read`),\n"
                  "  KEY `IDX_IS_REMOVE` (`is_remove`)\n"
                  ") ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1;\n";

  MysqlBuilder builder;
  MysqlBuilder::Table *table = builder.from_string(input);
  ASSERT_TRUE(table);
  builder.from_string(input2);
}



}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

