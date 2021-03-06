#include <my_global.h>
#include <mysql.h>

#define _POSIX_C_SOURCE 200809L

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define VALUE_TYPE_TEMP_CENTIGRADE 1
#define VALUE_TYPE_RELATIVE_HUMIDITY_PERCENT 2


#ifdef REAL_TIME
intmax_t get_current_time_in_ms (void)
{
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
clock_serv_t cclock;
mach_timespec_t mts;
host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
clock_get_time(cclock, &mts);
mach_port_deallocate(mach_task_self(), cclock);
spec.tv_sec = mts.tv_sec;
spec.tv_nsec = mts.tv_nsec;

#else
clock_gettime(CLOCK_MONOTONIC, &spec);
#endif

    s  = spec.tv_sec*1000 + round(spec.tv_nsec / 1.0e6);
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

    return (intmax_t)s;
}
#else
intmax_t get_current_time_in_ms (void)
{
	// fake implementation for performance tests
	static int t = 0;
	return t++;
}

#endif

#define BUFSIZE 1000

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);        
}

int main(int argc, char **argv)
{
	MYSQL_STMT *stmt;
	  MYSQL_BIND param[4];
	  char *sql = "insert into Measurements VALUES(?,?,?,?)";

	  int value, value_type, time_ms, sensor_id;
	  size_t data_length = sizeof(value);
	  int affected_rows;
	  
  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(con));
      exit(1);
  }  

  if (mysql_real_connect(con, "localhost", "till", NULL, 
          "till", 0, NULL, 0) == NULL) 
  {
      finish_with_error(con);
  }    
  
  if (mysql_query(con, "DROP TABLE IF EXISTS Measurements")) {
      finish_with_error(con);
  }
  
  if (mysql_query(con, "CREATE TABLE Measurements(value INT, time INT, valuetype INT, sensor_id INT) engine=myisam")) {      
      finish_with_error(con);
  }
  
  if (mysql_query(con, "INSERT INTO Measurements VALUES(1,1,1,1)")) {
      finish_with_error(con);
  }
  
  stmt = mysql_stmt_init(con);
  
    if (!stmt) {
      fprintf(stderr, " mysql_stmt_init(), out of memory\n");
      exit(EXIT_FAILURE);
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
      fprintf(stderr, " mysql_stmt_prepare(), INSERT failed\n");
      fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
      exit(EXIT_FAILURE);
    }
    
    memset(param, 0, sizeof(param));
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer = (char *)&value;
    param[0].buffer_length = sizeof(value);
    param[0].is_null = 0;
    param[0].length = &data_length;
	
    param[1].buffer_type = MYSQL_TYPE_LONG;
    param[1].buffer = (char *)&value_type;
    param[1].buffer_length = sizeof(value);
    param[1].is_null = 0;
    param[1].length = &data_length;
	
    param[2].buffer_type = MYSQL_TYPE_LONG;
    param[2].buffer = (char *)&time_ms;
    param[2].buffer_length = sizeof(value);
    param[2].is_null = 0;
    param[2].length = &data_length;
	
    param[3].buffer_type = MYSQL_TYPE_LONG;
    param[3].buffer = (char *)&sensor_id;
    param[3].buffer_length = sizeof(value);
    param[3].is_null = 0;
    param[3].length = &data_length;

	if (mysql_stmt_bind_param(stmt, param)) {
	    fprintf(stderr, " mysql_stmt_bind_param() failed\n");
	    fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
	    exit(EXIT_FAILURE);
	}	
	
	int iValues = 10000;
	int iSensors = 10;
	int i,j;
	intmax_t t;
	
	//printf("%ld\n", get_current_time_in_ms());
  	for(i=0;i<iValues;i++) {
  		t = get_current_time_in_ms();
		for(j=0;j<iSensors;j++) {
			value = i+j;
			value_type = VALUE_TYPE_TEMP_CENTIGRADE;
			time_ms = t;
			sensor_id = j;
			if (mysql_stmt_execute(stmt))
			{
				fprintf(stderr, " mysql_stmt_execute(), 1 failed\n");
				fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
				exit(EXIT_FAILURE);
			}
		}
  	}
	
 
  mysql_close(con);
  exit(0);
}
