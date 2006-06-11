// @(#)root/geom:$Name:  $:$Id: TGeoMaterial.cxx,v 1.30 2006/05/26 09:09:59 brun Exp $
// Author: Andrei Gheata   25/10/01

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// Full description with examples and pictures
//
//
//
//
//Begin_Html
/*
<img src="gif/t_material.jpg">
*/
//End_Html
#include "Riostream.h"
#include "TMath.h"
#include "TObjArray.h"
#include "TStyle.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"

// statics and globals

ClassImp(TGeoMaterial)

//-----------------------------------------------------------------------------
TGeoMaterial::TGeoMaterial()
{
// Default constructor
   SetUsed(kFALSE);
   fIndex    = -1;
   fShader   = 0;
   fA        = 0;
   fZ        = 0;
   fDensity  = 0;
   fRadLen   = 0;
   fIntLen   = 0;       
   fCerenkov = 0;
   fElement  = 0;
}
//-----------------------------------------------------------------------------
TGeoMaterial::TGeoMaterial(const char *name)
             :TNamed(name, "")
{
// constructor
   fName = fName.Strip();
   SetUsed(kFALSE);
   fIndex    = -1;
   fShader   = 0;
   fA        = 0;
   fZ        = 0;
   fDensity  = 0;
   fRadLen   = 0;
   fIntLen   = 0;
   fCerenkov = 0;
   fElement  = 0;
   
   if (!gGeoManager) {
      gGeoManager = new TGeoManager("Geometry", "default geometry");
   }
   gGeoManager->AddMaterial(this);
}
//-----------------------------------------------------------------------------
TGeoMaterial::TGeoMaterial(const char *name, Double_t a, Double_t z, 
                Double_t rho, Double_t radlen, Double_t intlen)
             :TNamed(name, "")
{
// constructor
   fName = fName.Strip();
   SetUsed(kFALSE);
   fShader   = 0;
   fIndex    = -1;
   fA        = a;
   fZ        = z;
   fDensity  = rho;
   fCerenkov = 0;
   fElement  = 0;
   SetRadLen(radlen, intlen);
   if (!gGeoManager) {
      gGeoManager = new TGeoManager("Geometry", "default geometry");
   }
   if (fZ - Int_t(fZ) > 1E-3)
      Warning("ctor", "Material %s defined with fractional Z=%f", GetName(), fZ);
   GetElement()->SetUsed();
   gGeoManager->AddMaterial(this);
}
//-----------------------------------------------------------------------------
TGeoMaterial::TGeoMaterial(const char *name, TGeoElement *elem,
                Double_t rho)
             :TNamed(name, "")
{
// constructor
   fName = fName.Strip();
   SetUsed(kFALSE);
   fShader   = 0;
   fIndex    = -1;
   fA        = elem->A();
   fZ        = elem->Z();
   fDensity  = rho;
   fCerenkov = 0;
   fElement  = elem;
   
   SetRadLen(0,0);
   if (!gGeoManager) {
      gGeoManager = new TGeoManager("Geometry", "default geometry");
   }
   if (fZ - Int_t(fZ) > 1E-3)
      Warning("ctor", "Material %s defined with fractional Z=%f", GetName(), fZ);
   GetElement()->SetUsed();
   gGeoManager->AddMaterial(this);
}
//-----------------------------------------------------------------------------
TGeoMaterial::TGeoMaterial(const TGeoMaterial& gm) :
  TNamed(gm),
  TAttFill(gm),
  fIndex(gm.fIndex),
  fA(gm.fA),
  fZ(gm.fZ),
  fDensity(gm.fDensity),
  fRadLen(gm.fRadLen),
  fIntLen(gm.fIntLen),
  fShader(gm.fShader),
  fCerenkov(gm.fCerenkov),
  fElement(gm.fElement)
{ 
   //copy constructor
}
//-----------------------------------------------------------------------------
TGeoMaterial& TGeoMaterial::operator=(const TGeoMaterial& gm) 
{
   //equal operator
   if(this!=&gm) {
      TNamed::operator=(gm);
      TAttFill::operator=(gm);
      fIndex=gm.fIndex;
      fA=gm.fA;
      fZ=gm.fZ;
      fDensity=gm.fDensity;
      fRadLen=gm.fRadLen;
      fIntLen=gm.fIntLen;
      fShader=gm.fShader;
      fCerenkov=gm.fCerenkov;
      fElement=gm.fElement;
   } 
   return *this;
}
//-----------------------------------------------------------------------------
TGeoMaterial::~TGeoMaterial()
{
// Destructor
}

//_____________________________________________________________________________
char *TGeoMaterial::GetPointerName() const
{
// Provide a pointer name containing uid.
   static char name[20];
   sprintf(name,"pMat%d", GetUniqueID());
   return name;
}    

//-----------------------------------------------------------------------------
void TGeoMaterial::SetRadLen(Double_t radlen, Double_t intlen)
{
// Set radiation/absorbtion lengths. If the values are negative, their absolute value
// is taken, otherwise radlen is recomputed using G3 formula.
   fRadLen = TMath::Abs(radlen);
   fIntLen = TMath::Abs(intlen);
   // compute radlen systematically with G3 formula for a valid material
   if (fA > 0 && fZ > 0 && radlen>=0) {
      //taken grom Geant3 routine GSMATE
      const Double_t alr2av=1.39621E-03, al183=5.20948;
      fRadLen = fA/(alr2av*fDensity*fZ*(fZ +TGeoMaterial::ScreenFactor(fZ))*
             (al183-TMath::Log(fZ)/3-TGeoMaterial::Coulomb(fZ)));
   } else {
      if (radlen>0) Error("SetRadLen","Invalid material %s: a=%f z=%f -> user values taken: radlen=%f intlen=%f",fA,fZ,radlen,intlen);
   }   
}   

//-----------------------------------------------------------------------------
Double_t TGeoMaterial::Coulomb(Double_t z)
{
   // static function
   //  Compute Coulomb correction for pair production and Brem 
   //  REFERENCE : EGS MANUAL SLAC 210 - UC32 - JUNE 78
   //                        FORMULA 2.7.17
   
   const Double_t alpha = 7.29927E-03;

   Double_t az    = alpha*z;
   Double_t az2   = az*az;
   Double_t az4   =   az2 * az2;
   Double_t fp    = ( 0.0083*az4 + 0.20206 + 1./(1.+az2) ) * az2;
   Double_t fm    = ( 0.0020*az4 + 0.0369  ) * az4;
   return fp - fm;
}


//-----------------------------------------------------------------------------
Bool_t TGeoMaterial::IsEq(const TGeoMaterial *other) const
{
// return true if the other material has the same physical properties
   if (other==this) return kTRUE;
   if (other->IsMixture()) return kFALSE;
   if (TMath::Abs(fA-other->GetA())>1E-3) return kFALSE;
   if (TMath::Abs(fZ-other->GetZ())>1E-3) return kFALSE;
   if (TMath::Abs(fDensity-other->GetDensity())>1E-6) return kFALSE;
   if (GetCerenkovProperties() != other->GetCerenkovProperties()) return kFALSE;
//   if (fRadLen != other->GetRadLen()) return kFALSE;
//   if (fIntLen != other->GetIntLen()) return kFALSE;
   return kTRUE;
}
//-----------------------------------------------------------------------------
void TGeoMaterial::Print(const Option_t * /*option*/) const
{
// print characteristics of this material
   printf("Material %s %s   A=%g Z=%g rho=%g radlen=%g index=%i\n", GetName(), GetTitle(),
          fA,fZ,fDensity, fRadLen, fIndex);
}

//_____________________________________________________________________________
void TGeoMaterial::SavePrimitive(ofstream &out, Option_t * /*option*/)
{
// Save a primitive as a C++ statement(s) on output stream "out".
   if (TestBit(TGeoMaterial::kMatSavePrimitive)) return;
   char *name = GetPointerName();
   out << "// Material: " << GetName() << endl;
   out << "   a       = " << fA << ";" << endl;
   out << "   z       = " << fZ << ";" << endl;
   out << "   density = " << fDensity << ";" << endl;
   out << "   radl    = " << fRadLen << ";" << endl;
   out << "   absl    = " << fIntLen << ";" << endl;
   
   out << "   " << name << " = new TGeoMaterial(\"" << GetName() << "\", a,z,density,radl,absl);" << endl;
   out << "   " << name << "->SetIndex(" << GetIndex() << ");" << endl;
   SetBit(TGeoMaterial::kMatSavePrimitive);
}

//-----------------------------------------------------------------------------
Int_t TGeoMaterial::GetDefaultColor() const
{
// Get some default color related to this material.
   Int_t id = 1+ gGeoManager->GetListOfMaterials()->IndexOf(this);
   return (2+id%6);
}

//-----------------------------------------------------------------------------
TGeoElement *TGeoMaterial::GetElement(Int_t) const
{
// Get a pointer to the element this material is made of.
   if (fElement) return fElement;
   TGeoElementTable *table = gGeoManager->GetElementTable();
   return table->GetElement(Int_t(fZ));
}

//-----------------------------------------------------------------------------
Int_t TGeoMaterial::GetIndex()
{
// Retreive material index in the list of materials
   if (fIndex>=0) return fIndex;
   TList *matlist = gGeoManager->GetListOfMaterials();
   fIndex = matlist->IndexOf(this);
   return fIndex;
}      
/*************************************************************************
 * TGeoMixture - mixtures of elements 
 *
 *************************************************************************/
ClassImp(TGeoMixture)

//-----------------------------------------------------------------------------
TGeoMixture::TGeoMixture()
{
// Default constructor
   fNelements  = 0;
   fZmixture   = 0;
   fAmixture   = 0;
   fWeights    = 0;
   fNatoms     = 0;
   fElements   = 0;
}
//-----------------------------------------------------------------------------
TGeoMixture::TGeoMixture(const char *name, Int_t /*nel*/, Double_t rho)
            :TGeoMaterial(name)
{
// constructor
   fZmixture   = 0;
   fAmixture   = 0;
   fWeights    = 0;
   fNelements  = 0;
   fNatoms     = 0;
   fDensity = rho;
   fElements   = 0;
   if (fDensity < 0) fDensity = 0.001;
}
//-----------------------------------------------------------------------------
TGeoMixture::TGeoMixture(const TGeoMixture& gm) :
  TGeoMaterial(gm),
  fNelements(gm.fNelements),
  fZmixture(gm.fZmixture),
  fAmixture(gm.fAmixture),
  fWeights(gm.fWeights),
  fNatoms(gm.fNatoms),
  fElements(gm.fElements)
{ 
   //copy constructor
}
//-----------------------------------------------------------------------------
TGeoMixture& TGeoMixture::operator=(const TGeoMixture& gm) 
{
   //equal operator
   if(this!=&gm) {
      TGeoMaterial::operator=(gm);
      fNelements=gm.fNelements;
      fZmixture=gm.fZmixture;
      fAmixture=gm.fAmixture;
      fWeights=gm.fWeights;
      fNatoms = gm.fNatoms;
      fElements = gm.fElements;
   } 
   return *this;
}
//-----------------------------------------------------------------------------
TGeoMixture::~TGeoMixture()
{
// Destructor
   if (fZmixture) delete[] fZmixture;
   if (fAmixture) delete[] fAmixture;
   if (fWeights)  delete[] fWeights;
   if (fNatoms)   delete[] fNatoms;
   if (fElements) delete fElements;
}
//-----------------------------------------------------------------------------
void TGeoMixture::AverageProperties()
{
// Compute effective A/Z and radiation length
   const Double_t alr2av = 1.39621E-03 , al183 =5.20948;
   Double_t radinv = 0;
   fA = 0;
   fZ = 0;
   for (Int_t j=0;j<fNelements;j++) {
      if (fWeights[j] <= 0) continue;
      fA += fWeights[j]*fAmixture[j];
      fZ += fWeights[j]*fZmixture[j];
      Double_t zc = fZmixture[j];
      Double_t alz = TMath::Log(zc)/3.;
      Double_t xinv = zc*(zc+TGeoMaterial::ScreenFactor(zc))*
         (al183-alz-TGeoMaterial::Coulomb(zc))/fAmixture[j];
      radinv += xinv*fWeights[j];
   }
   radinv *= alr2av*fDensity;
   if (radinv > 0) fRadLen = 1/radinv;
}

//-----------------------------------------------------------------------------
void TGeoMixture:: AddElement(Double_t a, Double_t z, Double_t weight)
{
// add an element to the mixture using fraction by weight
   // Check if the element is already defined
   Int_t i;
   for (i=0; i<fNelements; i++) {
      if (TMath::Abs(z-fZmixture[i])<1.e-6  && TMath::Abs(a-fAmixture[i])<1.e-6) {
         fWeights[i] += weight;
         AverageProperties();
         return;
      }
   }      
   if (!fNelements) {
      fZmixture = new Double_t[1];
      fAmixture = new Double_t[1];
      fWeights  = new Double_t[1];
   } else {   
      Int_t nelements = fNelements+1;
      Double_t *zmixture = new Double_t[nelements];
      Double_t *amixture = new Double_t[nelements];
      Double_t *weights  = new Double_t[nelements];
      for (Int_t j=0; j<fNelements; j++) {
         zmixture[j] = fZmixture[j];
         amixture[j] = fAmixture[j];
         weights[j]  = fWeights[j];
      }
      delete [] fZmixture;
      delete [] fAmixture;
      delete [] fWeights;
      fZmixture = zmixture;
      fAmixture = amixture;
      fWeights  = weights;
   }       
   
   fNelements++;
   i = fNelements - 1;   
   fZmixture[i] = z;
   fAmixture[i] = a;
   fWeights[i]  = weight;
   if (z - Int_t(z) > 1E-3)
      Warning("DefineElement", "Mixture %s has element defined with fractional Z=%f", GetName(), z);
   GetElement(i)->SetDefined();
   
   //compute equivalent radiation length (taken from Geant3/GSMIXT)
   AverageProperties();
}

//-----------------------------------------------------------------------------
void TGeoMixture::AddElement(TGeoMaterial *mat, Double_t weight)
{
// Define one component of the mixture as an existing material/mixture.
   TGeoElement *elnew, *elem;   
   Double_t a,z;
   if (!mat->IsMixture()) {
      elem = mat->GetBaseElement();
      if (elem) {
         AddElement(elem, weight);
      } else {   
         a = mat->GetA();
         z = mat->GetZ();
         AddElement(a, z, weight);
      }   
      return;
   }
   // The material is a mixture.
   TGeoMixture *mix = (TGeoMixture*)mat;
   Double_t wnew;
   Int_t nelem = mix->GetNelements();
   Bool_t elfound;
   Int_t i,j;
   // loop the elements of the daughter mixture
   for (i=0; i<nelem; i++) {
      elfound = kFALSE;
      elnew = mix->GetElement(i);
      if (!elnew) continue;
      // check if we have the element already defined in the parent mixture
      for (j=0; j<fNelements; j++) {
         if (fWeights[j]<=0) continue;
         elem = GetElement(j);
         if (elem == elnew) {
            // element found, compute new weight
            fWeights[j] += weight * (mix->GetWmixt())[i];
            elfound = kTRUE;
            break;
         }
      }
      if (elfound) continue;
      // element not found, define it
      wnew = weight * (mix->GetWmixt())[i];
      AddElement(elnew, wnew);
   }   
}         

//-----------------------------------------------------------------------------
void TGeoMixture::AddElement(TGeoElement *elem, Double_t weight)
{
// add an element to the mixture using fraction by weight
   TGeoElement *elemold;
   TGeoElementTable *table = gGeoManager->GetElementTable();
   if (!fElements) fElements = new TObjArray(10);
   Bool_t exist = kFALSE;
   // If previous elements were defined by A/Z, add corresponding TGeoElements
   for (Int_t i=0; i<fNelements; i++) {
      elemold = (TGeoElement*)fElements->At(i);
      if (!elemold) fElements->AddAt(elemold = table->GetElement((Int_t)fZmixture[i]), i);   
      if (elemold == elem) exist = kTRUE;
   }
   if (!exist) fElements->AddAt(elem, fNelements);   
   AddElement(elem->A(), elem->Z(), weight);
}   

//-----------------------------------------------------------------------------
void TGeoMixture::AddElement(TGeoElement *elem, Int_t natoms)
{
// Add a mixture element by number of atoms in the chemical formula.
   Int_t i,j;
   Double_t amol;
   TGeoElement *elemold;
   TGeoElementTable *table = gGeoManager->GetElementTable();
   if (!fElements) fElements = new TObjArray(10);
   // Check if the element is already defined
   for (i=0; i<fNelements; i++) {
      elemold = (TGeoElement*)fElements->At(i);
      if (!elemold) fElements->AddAt(table->GetElement((Int_t)fZmixture[i]), i);
      else if (elemold != elem) continue;
      if ((elem==elemold) || 
          (TMath::Abs(elem->Z()-fZmixture[i])<1.e-6 && TMath::Abs(elem->A()-fAmixture[i])<1.e-6)) {
         fNatoms[i] += natoms;
         amol = 0.;
         for (j=0; j<fNelements; j++) amol += fAmixture[j]*fNatoms[j];
         for (j=0; j<fNelements; j++) fWeights[j] = fNatoms[j]*fAmixture[j]/amol;
         AverageProperties();
         return;
      }
   }
   // New element      
   if (!fNelements) {
      fZmixture = new Double_t[1];
      fAmixture = new Double_t[1];
      fWeights  = new Double_t[1];
      fNatoms   = new Int_t[1];
   } else {   
      if (!fNatoms) {
         Error("AddElement", "Cannot add element by natoms in mixture %s after defining elements by weight",
               GetName());
         return;
      }         
      Int_t nelements = fNelements+1;
      Double_t *zmixture = new Double_t[nelements];
      Double_t *amixture = new Double_t[nelements];
      Double_t *weights  = new Double_t[nelements];
      Int_t *nnatoms  = new Int_t[nelements];
      for (Int_t j=0; j<fNelements; j++) {
         zmixture[j] = fZmixture[j];
         amixture[j] = fAmixture[j];
         weights[j]  = fWeights[j];
         nnatoms[j]  = fNatoms[j];
      }
      delete [] fZmixture;
      delete [] fAmixture;
      delete [] fWeights;
      delete [] fNatoms;
      fZmixture = zmixture;
      fAmixture = amixture;
      fWeights  = weights;
      fNatoms   = nnatoms;
   }
   fNelements++;       
   Int_t iel = fNelements-1;
   fZmixture[iel] = elem->Z();
   fAmixture[iel] = elem->A();
   fNatoms[iel]  = natoms;
   fElements->AddAt(elem, iel);
   amol = 0.;
   for (i=0; i<fNelements; i++) {
      if (fNatoms[i]<=0) return;
      amol += fAmixture[i]*fNatoms[i];
   }   
   for (i=0; i<fNelements; i++) fWeights[i] = fNatoms[i]*fAmixture[i]/amol;
   AverageProperties();
}          

//-----------------------------------------------------------------------------
void TGeoMixture::DefineElement(Int_t /*iel*/, Int_t z, Int_t natoms)
{
// Define the mixture element at index iel by number of atoms in the chemical formula.
   TGeoElementTable *table = gGeoManager->GetElementTable();
   TGeoElement *elem = table->GetElement(z);
   if (!elem) Fatal("DefineElement", "In mixture %s, element with Z=%i not found",GetName(),z);
   AddElement(elem, natoms);
}
   
//-----------------------------------------------------------------------------
TGeoElement *TGeoMixture::GetElement(Int_t i) const
{
// Retreive the pointer to the element corresponding to component I.
   if (i<0 || i>=fNelements) {
      Error("GetElement", "Mixture %s has only %d elements", GetName(), fNelements);
      return 0;
   }   
   TGeoElement *elem = 0;
   if (fElements) elem = (TGeoElement*)fElements->At(i);
   if (elem) return elem;
   TGeoElementTable *table = gGeoManager->GetElementTable();
   return table->GetElement(Int_t(fZmixture[i]));
}

//-----------------------------------------------------------------------------
Bool_t TGeoMixture::IsEq(const TGeoMaterial *other) const
{
// Return true if the other material has the same physical properties
   if (other->IsEqual(this)) return kTRUE;
   if (!other->IsMixture()) return kFALSE;
   TGeoMixture *mix = (TGeoMixture*)other;
   if (!mix) return kFALSE;
   if (fNelements != mix->GetNelements()) return kFALSE;
   if (TMath::Abs(fA-other->GetA())>1E-3) return kFALSE;
   if (TMath::Abs(fZ-other->GetZ())>1E-3) return kFALSE;
   if (TMath::Abs(fDensity-other->GetDensity())>1E-6) return kFALSE;
   if (GetCerenkovProperties() != other->GetCerenkovProperties()) return kFALSE;
//   if (fRadLen != other->GetRadLen()) return kFALSE;
//   if (fIntLen != other->GetIntLen()) return kFALSE;
   for (Int_t i=0; i<fNelements; i++) {
      if (TMath::Abs(fZmixture[i]-(mix->GetZmixt())[i])>1E-3) return kFALSE;
      if (TMath::Abs(fAmixture[i]-(mix->GetAmixt())[i])>1E-3) return kFALSE;
      if (TMath::Abs(fWeights[i]-(mix->GetWmixt())[i])>1E-3) return kFALSE;
   }
   return kTRUE;
}
//-----------------------------------------------------------------------------
void TGeoMixture::Print(const Option_t * /*option*/) const
{
// print characteristics of this material
   printf("Mixture %s %s   Aeff=%g Zeff=%g rho=%g radlen=%g index=%i\n", GetName(), GetTitle(),
          fA,fZ,fDensity, fRadLen, fIndex);
   for (Int_t i=0; i<fNelements; i++) {
      if (fNatoms) printf("   Element #%i : Z=%6.2f A=%6.2f w=%6.2f natoms=%i\n", i, fZmixture[i],
             fAmixture[i], fWeights[i], fNatoms[i]);
      else printf("   Element #%i : Z=%6.2f A=%6.2f w=%6.2f\n", i, fZmixture[i],
             fAmixture[i], fWeights[i]);
   }
}

//_____________________________________________________________________________
void TGeoMixture::SavePrimitive(ofstream &out, Option_t * /*option*/)
{
// Save a primitive as a C++ statement(s) on output stream "out".
   if (TestBit(TGeoMaterial::kMatSavePrimitive)) return;
   char *name = GetPointerName();
   out << "// Mixture: " << GetName() << endl;
   out << "   nel     = " << fNelements << ";" << endl;
   out << "   density = " << fDensity << ";" << endl;
   out << "   " << name << " = new TGeoMixture(\"" << GetName() << "\", nel,density);" << endl;
   for (Int_t i=0; i<fNelements; i++) {
      TGeoElement *el = GetElement(i);
      out << "      a = " << fAmixture[i] << ";   z = "<< fZmixture[i] << ";   w = " << fWeights[i] << ";  // " << el->GetName() << endl;
      out << "   " << name << "->DefineElement(" << i << ",a,z,w);" << endl;
   }         
   out << "   " << name << "->SetIndex(" << GetIndex() << ");" << endl;
   SetBit(TGeoMaterial::kMatSavePrimitive);
}

//-----------------------------------------------------------------------------
Double_t TGeoMaterial::ScreenFactor(Double_t z)
{
   // static function
   //  Compute screening factor for pair production and Bremstrahlung
   //  REFERENCE : EGS MANUAL SLAC 210 - UC32 - JUNE 78
   //                        FORMULA 2.7.22
   
   const Double_t al183= 5.20948 , al1440 = 7.27239;
   Double_t alz  = TMath::Log(z)/3.;
   Double_t factor = (al1440 - 2*alz) / (al183 - alz - TGeoMaterial::Coulomb(z));
   return factor;
}


