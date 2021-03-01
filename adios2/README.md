# clangwrap
Clang-based wrapper for C++ libraries

This example is a little more complicated.  This example shows how to use the wrapper
generator with a complicated C++ library like ADIOS2.  This example assumes you have
an installation of ADIOS2, with MPI enabled.  To run this example, you still need the
clang compiler to build `tau_wrap++`, but you will also need a working MPI installation
and TAU will need to be configured a little differently.

```bash
git clone https://github.com/UO-OACISS/tau2.git
cd tau2
# Configure with MPI and pthread settings - no other settings necessary
./configure -mpi -pthread
make -j install
TAU_ARCH=`./utils/archfind`
export TAU_MAKEFILE=`pwd`/${TAU_ARCH}/lib/Makefile.tau-gnu-mpi-pthread
```

To build this example, first build `tau_wrap++` in the main `src` directory.

Next, you have to copy the ADIOS2 headers locally (to this directory), and modify them slightly.
The reason this is necessary is that the ADIOS2 headers contain explicit template
instantiations.  These instantiations prevent us from compiling specializations
for the templates.  That's OK, because the modified headers are only necessary to
compile the library wrapper.

To modify the headers, do the following.  Assuming the ADIOS2 headers are in
`/usr/local/packages/ADIOS2/2021.02.05-Release/mpi`:

```
[khuck@cyclops adios2]$ ADIOS2_ROOT=/usr/local/packages/ADIOS2/2021.02.05-Release/mpi
[khuck@cyclops adios2]$ cp -r ${ADIOS2_ROOT}/include .
```

...then modify any header that has sections with `declare_template_instantiation` macros
in it.  Those headers include (but may not be limited to): `Engine.h`, `Group.h`,
`IO.h`, `Types.h`, and `ADIOS2fstream.h`.  Comment out or remove those sections
from your local copy of the headers.

Then you can build this example, which will use the modified ADIOS2 headers as input,
generate the wrapper source code, and then build the wrapper library.
To run the example, just modify your `mpirun` statement to use `tau_exec`,
like this:

```bash
mpirun -np 4 tau_exec -T gnu,mpi,pthread -loadlib=/path/to/libadios2_wrap.so \
./build/gray-scott ./simulation/settings-files.json
```

...assuming your original mpirun statement was
`mpirun -np 4 ./build/gray-scott ./simulation/settings-files.json`.

Your output should look something like this:

```
Simulation writes data using engine type:              BP4
========================================
grid:             64x64x64
steps:            1000
plotgap:          10
F:                0.01
k:                0.05
dt:               2
Du:               0.2
Dv:               0.1
noise:            1e-07
output:           gs.bp
adios_config:     adios2.xml
process layout:   2x2x1
local grid size:  32x32x64
========================================
Simulation at step 10 writing output step     1
Simulation at step 20 writing output step     2
Simulation at step 30 writing output step     3
Simulation at step 40 writing output step     4
Simulation at step 50 writing output step     5
...
Simulation at step 1000 writing output step     100
[khuck@cyclops gray-scott]$ pprof -s -a
Reading Profile files in profile.*

FUNCTION SUMMARY (total):
---------------------------------------------------------------------------------------
%Time    Exclusive    Inclusive       #Call      #Subrs  Inclusive Name
              msec   total msec                          usec/call
---------------------------------------------------------------------------------------
100.0     2:38.281     3:14.955           4       50217   48738871 .TAU application
 14.6       28,489       28,489       48000           0        594 MPI_Sendrecv()
  3.0            2        5,881         400         400      14704 [WRAPPER] void adios2::Engine::EndStep()
  3.0           17        5,879         400         856      14698 BP4Writer::EndStep
  2.4           12        4,679         400         800      11699 BP4Writer::Flush
  2.3          336        4,578         404       12884      11332 BP4Writer::AggregateWriteData
  2.2        4,218        4,218        3226           0       1308 MPI_Wait()
  0.6        1,225        1,225           4           0     306436 MPI_Init()
  0.6        1,173        1,182         400        1600       2955 BP4Writer::PerformPuts
  0.4          719          719           4           0     179907 MPI_Finalize()
  0.1            7          132           4          16      33106 [WRAPPER] void adios2::Engine::Close(const int transportIndex)
  0.1          116          124           4          12      31227 BP4Writer::Close
  0.1          120          120         400           0        300 [WRAPPER] void adios2::Engine::Put<int>(Variable<int> variable, const int * data, const adios2::Mode launch)
  0.0           86           92         404        4200        229 BP4Writer::WriteCollectiveMetadataFile
  0.0           34           34         800           0         43 [WRAPPER] void adios2::Engine::Put<double>(Variable<double> variable, const double * data, const adios2::Mode launch)
  0.0        0.202           34           4           8       8514 [WRAPPER] adios2::Engine adios2::IO::Open(const std::string & name, const adios2::Mode mode)
  0.0            6           31           4          12       7820 IO::Open
  0.0           21           25           4          83       6272 BP4Writer::Open
  0.0           13           13        1613           0          8 MPI_Irecv()
  0.0            9           11         400         400         29 [WRAPPER] adios2::StepStatus adios2::Engine::BeginStep()
  0.0            7            7        1613           0          4 MPI_Isend()
  0.0            6            6           4           0       1624 MPI_Cart_create()
  0.0            6            6         816           0          8 IO::other
  0.0            5            5          24           0        229 IO::DefineVariable
  0.0            5            5           4          25       1326 BP4Writer::WriteProfilingJSONFile
  0.0            5            5           8           0        658 MPI_Comm_split()
  0.0        0.182            4          16          16        289 [WRAPPER] Variable<double> adios2::IO::DefineVariable<double>(const std::string & name, const adios2::Dims & shape, const adios2::Dims & start, const adios2::Dims & count, const bool constantDims)
  0.0            3            3         800           0          4 IO::InquireVariable
  0.0         0.31            3          48          48         71 [WRAPPER] Attribute<double> adios2::IO::DefineAttribute<double>(const std::string & name, const double & value, const std::string & variableName, const std::string separator)
  0.0            3            3           8           0        408 MPI_Comm_dup()
  0.0            3            3          56           0         57 IO::DefineAttribute
  0.0            2            2           4           0        740 MPI_Barrier()
  0.0            2            2         404           0          6 MPI_Gather()
  0.0            2            2         400           0          6 BP4Writer::BeginStep
  0.0            2            2          20           0        114 MPI_Bcast()
  0.0            1            1        3226           0          1 MPI_Get_count()
  0.0            1            1         404           0          5 MPI_Gatherv()
  0.0            1            1        3226           0          0 MPI_Test_cancelled()
  0.0            1            1        2462           0          1 MPI_Comm_rank()
  0.0        0.101            1           8           8        142 [WRAPPER] Variable<int> adios2::IO::DefineVariable<int>(const std::string & name, const adios2::Dims & shape, const adios2::Dims & start, const adios2::Dims & count, const bool constantDims)
  0.0        0.554        0.554         922           0          1 MPI_Comm_size()
  0.0        0.461        0.461           8           0         58 [WRAPPER] adios2::IO adios2::ADIOS::DeclareIO(const std::string name)
  0.0        0.342        0.342         100           0          3 BP4Writer::PopulateMetadataIndexFileContent
  0.0        0.319        0.319          12           0         27 MPI_Comm_free()
  0.0        0.231        0.231          56           0          4 IO::InquireAttribute
  0.0        0.096        0.096          12           0          8 MPI_Type_vector()
  0.0        0.062        0.062          16           0          4 MPI_Finalized()
  0.0         0.03         0.03           4           0          8 MPI_Dims_create()
  0.0        0.029        0.029           4           0          7 MPI_Info_create()
  0.0        0.024        0.024          12           0          2 MPI_Type_commit()
  0.0         0.02         0.02           4           0          5 MPI_Cart_coords()
  0.0        0.018        0.018          12           0          2 MPI_Cart_shift()
  0.0        0.004        0.004           1           0          4 [WRAPPER] std::string adios2::IO::EngineType() const

FUNCTION SUMMARY (mean):
---------------------------------------------------------------------------------------
%Time    Exclusive    Inclusive       #Call      #Subrs  Inclusive Name
              msec   total msec                          usec/call
---------------------------------------------------------------------------------------
100.0       39,570       48,738           1     12554.2   48738871 .TAU application
 14.6        7,122        7,122       12000           0        594 MPI_Sendrecv()
  3.0        0.567        1,470         100         100      14704 [WRAPPER] void adios2::Engine::EndStep()
  3.0            4        1,469         100         214      14698 BP4Writer::EndStep
  2.4            3        1,169         100         200      11699 BP4Writer::Flush
  2.3           84        1,144         101        3221      11332 BP4Writer::AggregateWriteData
  2.2        1,054        1,054       806.5           0       1308 MPI_Wait()
  0.6          306          306           1           0     306436 MPI_Init()
  0.6          293          295         100         400       2955 BP4Writer::PerformPuts
  0.4          179          179           1           0     179907 MPI_Finalize()
  0.1            1           33           1           4      33106 [WRAPPER] void adios2::Engine::Close(const int transportIndex)
  0.1           29           31           1           3      31227 BP4Writer::Close
  0.1           30           30         100           0        300 [WRAPPER] void adios2::Engine::Put<int>(Variable<int> variable, const int * data, const adios2::Mode launch)
  0.0           21           23         101        1050        229 BP4Writer::WriteCollectiveMetadataFile
  0.0            8            8         200           0         43 [WRAPPER] void adios2::Engine::Put<double>(Variable<double> variable, const double * data, const adios2::Mode launch)
  0.0       0.0505            8           1           2       8514 [WRAPPER] adios2::Engine adios2::IO::Open(const std::string & name, const adios2::Mode mode)
  0.0            1            7           1           3       7820 IO::Open
  0.0            5            6           1       20.75       6272 BP4Writer::Open
  0.0            3            3      403.25           0          8 MPI_Irecv()
  0.0            2            2         100         100         29 [WRAPPER] adios2::StepStatus adios2::Engine::BeginStep()
  0.0            1            1      403.25           0          4 MPI_Isend()
  0.0            1            1           1           0       1624 MPI_Cart_create()
  0.0            1            1         204           0          8 IO::other
  0.0            1            1           6           0        229 IO::DefineVariable
  0.0            1            1           1        6.25       1326 BP4Writer::WriteProfilingJSONFile
  0.0            1            1           2           0        658 MPI_Comm_split()
  0.0       0.0455            1           4           4        289 [WRAPPER] Variable<double> adios2::IO::DefineVariable<double>(const std::string & name, const adios2::Dims & shape, const adios2::Dims & start, const adios2::Dims & count, const bool constantDims)
  0.0        0.865        0.865         200           0          4 IO::InquireVariable
  0.0       0.0775        0.851          12          12         71 [WRAPPER] Attribute<double> adios2::IO::DefineAttribute<double>(const std::string & name, const double & value, const std::string & variableName, const std::string separator)
  0.0        0.815        0.815           2           0        408 MPI_Comm_dup()
  0.0        0.804        0.804          14           0         57 IO::DefineAttribute
  0.0         0.74         0.74           1           0        740 MPI_Barrier()
  0.0        0.625        0.625         101           0          6 MPI_Gather()
  0.0        0.615        0.615         100           0          6 BP4Writer::BeginStep
  0.0        0.572        0.572           5           0        114 MPI_Bcast()
  0.0        0.471        0.471       806.5           0          1 MPI_Get_count()
  0.0        0.462        0.462         101           0          5 MPI_Gatherv()
  0.0        0.401        0.401       806.5           0          0 MPI_Test_cancelled()
  0.0        0.367        0.367       615.5           0          1 MPI_Comm_rank()
  0.0       0.0253        0.285           2           2        142 [WRAPPER] Variable<int> adios2::IO::DefineVariable<int>(const std::string & name, const adios2::Dims & shape, const adios2::Dims & start, const adios2::Dims & count, const bool constantDims)
  0.0        0.139        0.139       230.5           0          1 MPI_Comm_size()
  0.0        0.115        0.115           2           0         58 [WRAPPER] adios2::IO adios2::ADIOS::DeclareIO(const std::string name)
  0.0       0.0855       0.0855          25           0          3 BP4Writer::PopulateMetadataIndexFileContent
  0.0       0.0798       0.0798           3           0         27 MPI_Comm_free()
  0.0       0.0578       0.0578          14           0          4 IO::InquireAttribute
  0.0        0.024        0.024           3           0          8 MPI_Type_vector()
  0.0       0.0155       0.0155           4           0          4 MPI_Finalized()
  0.0       0.0075       0.0075           1           0          8 MPI_Dims_create()
  0.0      0.00725      0.00725           1           0          7 MPI_Info_create()
  0.0        0.006        0.006           3           0          2 MPI_Type_commit()
  0.0        0.005        0.005           1           0          5 MPI_Cart_coords()
  0.0       0.0045       0.0045           3           0          2 MPI_Cart_shift()
  0.0        0.001        0.001        0.25           0          4 [WRAPPER] std::string adios2::IO::EngineType() const
```
