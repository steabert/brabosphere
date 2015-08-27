TEMPLATE = lib
CONFIG += release exceptions
CONFIG -= qt
INCLUDEPATH += src
OBJECTS_DIR = obj
DESTDIR = lib

HEADERS += src\aromatic.h \
           src\atomtyp.h \
           src\babelconfig.h \
           src\base.h \
           src\binary.h \
           src\bitvec.h \
           src\bondtyp.h \
           src\bondtyper.h \
           src\chains.h \
           src\chiral.h \
           src\crk.h \
           src\data.h \
           src\element.h \
           src\extable.h \
           src\fileformat.h \
           src\generic.h \
           src\grid.h \
           src\isotope.h \
           src\matrix.h \
           src\mol.h \
           src\molchrg.h \
           src\molvector.h \
           src\oberror.h \
           src\obifstream.h \
           src\obutil.h \
           src\parsmart.h \
           src\patty.h \
           src\phmodel.h \
           src\phmodeldata.h \
           src\resdata.h \
           src\ring.h \
           src\rotor.h \
           src\smi.h \
           src\snprintf.h \
           src\typer.h \
           src\types.h \
           src\math\matrix3x3.h \
           src\math\vector3.h
           
SOURCES += src\alchemy.cpp \
           src\amber.cpp \
           src\atom.cpp \
           src\balst.cpp \
           src\base.cpp \
           src\bgf.cpp \
           src\binary.cpp \
           src\bitvec.cpp \
           src\bond.cpp \
           src\box.cpp \
           src\c3d.cpp \
           src\cacao.cpp \
           src\cache.cpp \
           src\car.cpp \
           src\ccc.cpp \
           src\chains.cpp \
           src\chdrw.cpp \
           src\chemtool.cpp \
           src\chiral.cpp \
           src\cml.cpp \
           src\crk.cpp \
           src\csr.cpp \
           src\cssr.cpp \
           src\data.cpp \
           src\dmol.cpp \
           src\feat.cpp \
           src\fh.cpp \
           src\fileformat.cpp \
           src\gamess.cpp \
           src\gaussian.cpp \
           src\generic.cpp \
           src\ghemical.cpp \
           src\grid.cpp \
           src\gromos96.cpp \
           src\hin.cpp \
           src\jaguar.cpp \
           src\matrix.cpp \
           src\mdl.cpp \
           src\mm3.cpp \
           src\mmod.cpp \
           src\mol.cpp \
           src\mol2.cpp \
           src\molchrg.cpp \
           src\molvector.cpp \
           src\mopac.cpp \
           src\mpqc.cpp \
           src\nwchem.cpp \
           src\oberror.cpp \
           src\obutil.cpp \
           src\parsmart.cpp \
           src\parsmi.cpp \
           src\patty.cpp \
           src\pdb.cpp \
           src\phmodel.cpp \
           src\povray.cpp \
           src\pqs.cpp \
           src\qchem.cpp \
           src\rand.cpp \
           src\report.cpp \
           src\residue.cpp \
           src\ring.cpp \
           src\rotor.cpp \
           src\shelx.cpp \
           src\smi.cpp \
           src\tinker.cpp \
           src\tokenst.cpp \
           src\typer.cpp \
           src\unichem.cpp \
           src\viewmol.cpp \
           src\xed.cpp \
           src\xyz.cpp \
           src\zindo.cpp \
           src\math\matrix3x3.cpp \
           src\math\vector3.cpp
