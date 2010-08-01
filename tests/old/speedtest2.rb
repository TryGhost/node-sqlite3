require 'sqlite3'

db = SQLite3::Database.new(":memory:")


db.execute("CREATE TABLE t1 (alpha INTEGER, beta TEXT, pi FLOAT)") do |row|

end

count = 100000;

t0 = Time.new;
1.upto(count) do
  db.execute("INSERT INTO t1 VALUES (1, 'hello', 3.141)") do
  end
end
d = Time.new - t0;
puts "Took #{d}s (#{count/d} inserts/s)";

t0 = Time.new;
db.execute("SELECT * FROM t1") do |row|
end
d = Time.new - t0;
puts "Took #{d} (#{count/d} rows/s)";
