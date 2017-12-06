#include "vtk_writer.hpp"

void af_to_vtk(const af::array field, const Mesh& mesh, std::string outputname){
//void af_to_vtk(const State& a){

  const double d[3]={mesh.dx,mesh.dy,mesh.dz};
  af::dim4 dims(field.dims(0),field.dims(1),field.dims(2),field.dims(3));

  double* host_a = field.host<double>();

  std::cout<<"vtk_writer: Number of points:"<< dims[0]*dims[1]*dims[2]*dims[3]<<std::endl;

  //------------------------------------------------------------------------------
  //VKT grid
  //------------------------------------------------------------------------------
  vtkSmartPointer<vtkRectilinearGrid> grid =
    vtkSmartPointer<vtkRectilinearGrid>::New();

  grid->SetDimensions(dims[0], dims[1], dims[2]);

  vtkDataArray* coords[3];

  // compute & populate coordinate vectors
  for(int i=0; i < dims[3]; ++i)//i^= x,y,z
  {
    coords[i] = vtkDataArray::CreateDataArray(VTK_DOUBLE);
    coords[i]->SetNumberOfTuples(dims[i]);

    for(int j=0; j < dims[i]; ++j)
    {
      double val = (double)j*d[i];
      coords[ i ]->SetTuple( j, &val );
    } // END for all points along this dimension

  } // END for all dimensions

  grid->SetXCoordinates( coords[0] );
  grid->SetYCoordinates( coords[1] );
  grid->SetZCoordinates( coords[2] );
  coords[0]->Delete();
  coords[1]->Delete();
  coords[2]->Delete();

  //VKT value grid
  vtkSmartPointer<vtkRectilinearGrid> value_grid =
    vtkSmartPointer<vtkRectilinearGrid>::New();
  value_grid->SetDimensions(1,1,1);
  vtkDataArray* value_coords[3];

  for(int i=0; i < 3; ++i)//i^= x,y,z
  {
    value_coords[i] = vtkDataArray::CreateDataArray(VTK_DOUBLE);
    value_coords[i]->SetNumberOfTuples(1);
  } // END for all dimensions

  // compute & populate XYZ field
  vtkIdType npoints = grid->GetNumberOfPoints();
  vtkDoubleArray* vkt_xyz = vtkDoubleArray::New();
  vkt_xyz->SetName("m");
  //vkt_xyz->SetName( outputname.c_str() );
  vkt_xyz->SetNumberOfComponents(3);
  vkt_xyz->SetNumberOfTuples( npoints );

  for(vtkIdType pntIdx=0; pntIdx < npoints; ++pntIdx )
  {
    double af_vals[3];
    for(int i=0; i < 3; ++i)
    {
      af_vals[i]=host_a[pntIdx+i*npoints];
      value_coords[i]->SetTuple(0, &af_vals[i] );
    }
      value_grid->SetXCoordinates( value_coords[0] );
      value_grid->SetYCoordinates( value_coords[1] );
      value_grid->SetZCoordinates( value_coords[2] );

      vkt_xyz->SetTuple(pntIdx, value_grid->GetPoint(0) );
  } // END for all points
  grid->GetPointData()->AddArray( vkt_xyz );

//   vtkNew<vtkPointDataToCellData> pd2cd;
//   pd2cd->PassPointDataOn();
//   pd2cd->SetInputDataObject(grid);
//   pd2cd->Update();
//   grid->ShallowCopy(pd2cd->GetOutputDataObject(0));
  //Write into Test.vtk
  //
  vtkRectilinearGridWriter* writer = vtkRectilinearGridWriter::New();
  
  writer->SetFileName((outputname.append(".vtk")).c_str());
  //writer->SetFileName( oss.str().c_str() );
  writer->SetInputData( grid );
  writer->Write();
  writer->Delete();
  vkt_xyz->Delete();

  value_coords[0]->Delete();
  value_coords[1]->Delete();
  value_coords[2]->Delete();

  delete[] host_a;
}


//3D Image data Cell Centered
//Optimization should avoid generation of two vtkImageData objects
void af_to_vti(const af::array field, const Mesh& mesh, std::string outputname){
  
    double* host_a = field.host<double>();
    for(int i=0; i < field.dims(0)* field.dims(1)* field.dims(2) * field.dims(3); i++){
        std::cout<<"host: "<< i << " = " << host_a[i]<<std::endl;
    }
    af::array Test(field.dims(0), field.dims(1), field.dims(2) , field.dims(3),host_a);
    af::print("Test=",Test);
  
    std::cout<<"vtk_writer: af_to_vti: Number of af::array elements:"<< field.dims(0)* field.dims(1)* field.dims(2) * field.dims(3)<<std::endl;
  
    vtkSmartPointer<vtkImageData> imageDataPointCentered = vtkSmartPointer<vtkImageData>::New();
    imageDataPointCentered->SetDimensions(field.dims(0), field.dims(1), field.dims(2));
    imageDataPointCentered->SetSpacing(mesh.dx,mesh.dy,mesh.dz);
    imageDataPointCentered->SetOrigin(0,0,0);
    #if VTK_MAJOR_VERSION <= 5
       imageDataPointCentered->SetNumberOfScalarComponents(field.dims(3));
       imageDataPointCentered->SetScalarTypeToDouble();
    #else
       imageDataPointCentered->AllocateScalars(VTK_DOUBLE, field.dims(3));
    #endif
    int* dims = imageDataPointCentered->GetDimensions();
  
    for (int z = 0; z < dims[2]; z++)
      {
      for (int y = 0; y < dims[1]; y++)
        {
        for (int x = 0; x < dims[0]; x++)
          {
          for (int im=0; im < field.dims(3); im++)
            {
            double* pixel = static_cast<double*>(imageDataPointCentered->GetScalarPointer(x,y,z));
            pixel[im] = host_a[x+dims[0]*(y+dims[1]*(z+ dims[2] * im))];
            //int idx = x+dims[0]*(y+dims[1]*(z+ dims[2] * im));
            //std::cout<<"idx= "<< idx<< "\t" <<host_a[idx]<<std::endl;                                               
            }
          }
        }
      }

    vtkSmartPointer<vtkImageData> imageDataCellCentered = vtkSmartPointer<vtkImageData>::New();
    imageDataCellCentered->SetDimensions(field.dims(0)+1, field.dims(1)+1, field.dims(2)+1);
    imageDataCellCentered->SetOrigin(0,0,0);
    imageDataCellCentered->SetSpacing(mesh.dx,mesh.dy,mesh.dz);
    imageDataCellCentered->GetCellData()->SetScalars (imageDataPointCentered->GetPointData()->GetScalars());
  
    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName((outputname.append(".vti")).c_str());
    #if VTK_MAJOR_VERSION <= 5
        writer->SetInputConnection(imageDataCellCentered->GetProducerPort());
    #else
        writer->SetInputData(imageDataCellCentered);
    #endif
    writer->Write();
}

//https://www.vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/DataManipulation/Cxx/Arrays.cxx

void vti_to_af(const af::array& field, const Mesh& mesh, std::string filepath){
    vtkSmartPointer<vtkXMLImageDataReader> reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
    reader->SetFileName(filepath.c_str());
    reader->Update(); 
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    if(reader->GetOutput()==NULL) std::cout<<" reader==NULL"<<std::endl;
    imageData=reader->GetOutput();
//TODO    Assert()
    assert (imageData!=NULL);
    std::cout<<"---->VTK: GetNumberOfCells() "<<imageData->GetNumberOfCells()<<std::endl;
    std::cout<<"---->VTK: GetNumberOfPoints()"<<imageData->GetNumberOfPoints()<<std::endl;
    std::cout<<"---->VTK: GetNumberOfPoints()"<<imageData->GetCellData()->GetScalars()<<std::endl;
    //double * temp = imageData->GetCellData()->GetScalars();
    //std::cout<<"---->VTK: "<<temp[0]<<std::endl;
    
    double* pixel = static_cast<double*>(imageData->GetScalarPointer(0,0,0));
    //vtkCell* cell = imageData->GetCell(0);
    //double* temp = cell->GetPoints()->GetPoint(0);
    std::cout<<"---->VTK------------------------: "<<pixel  <<std::endl;
    //std::cout<<"---->VTK: "<<pixel[0]  <<std::endl;
    //double* temp = imageData->GetCell(0)->GetPoints();
    vtkSmartPointer<vtkDoubleArray> temp = vtkSmartPointer<vtkDoubleArray>::New();
    //vtkSmartPointer<vtkDataArray> temp1 = vtkSmartPointer<vtkDataArray>::New();
    imageData->GetCellData()->GetScalars()->GetData(0,3*imageData->GetNumberOfCells(),0,2,temp);
    int* dims_reduced = imageData->GetDimensions();
    for(int i=0; i < 3; i++){
        std::cout<<"dims_reduced= "<< i << " = " <<  dims_reduced[i]<<std::endl;
        dims_reduced[i]--;
        std::cout<<"dims_reduced= "<< i << " = " <<  dims_reduced[i]<<std::endl;
    }
    double* A_host = NULL;
    A_host = new double[3*imageData->GetNumberOfCells()];
    double* B_host = NULL;
    B_host = new double[3*imageData->GetNumberOfCells()];
    for(int i=0; i < 3* imageData->GetNumberOfCells(); i++){
        std::cout<<"i = "<< i << "     Value = " <<  temp->GetValue(i)<<std::endl;
        A_host[i]=temp->GetValue(i);
    }
    for (int z = 0; z < dims_reduced[2]; z++)
      {
      for (int y = 0; y < dims_reduced[1]; y++)
        {
        for (int x = 0; x < dims_reduced[0]; x++)
          {
          for (int im=0; im < 3; im++)//TODO make this dimension depemdent on input
            {
            //  A_host[i]=temp->GetValue(i);
            //double* pixel = static_cast<double*>(imageDataPointCentered->GetScalarPointer(x,y,z));
            //pixel[im] = host_a[x+dims_reduced[0]*(y+dims_reduced[1]*(z+ dims_reduced[2] * im))];
            //int idx = im+dims_reduced[2]*(z+dims_reduced[1]*(y+ dims_reduced[0] * x));
            int idx = x+dims_reduced[0]*(y+dims_reduced[1]*(z+ dims_reduced[2] * im));
            //int idx = im * dims_reduced[0] * dims_reduced [1] * dims_reduced[2] ;
            std::cout<<"idx= "<< idx<< " \t A_host[idx] =  " <<A_host[idx] << "\t GetValue= "<< temp->GetValue(idx)<<std::endl;
            B_host[idx]=temp->GetValue(idx);
            }
          }
        }
      }
    af::array A(3*imageData->GetNumberOfCells(),1,1,1,A_host);
    delete [] A_host;
    af::print("A=",A);
    A=af::moddims(A,af::dim4(3,dims_reduced[0],dims_reduced[1],dims_reduced[2]));
    af::print("A=",A);
    A=af::reorder(A,1,2,3,0);
    af::print("A=",A);
    
    std::cout<<"TYPE="<<A.type()<<std::endl;
    //af::array B=af::constant(2.,1,1,1,1,f64);
    //std::cout<<"TYPE="<<B.type()<<std::endl;
    
    //std::cout<<"lalalal "<< temp->GetValue(0)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(1)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(2)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(3)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(4)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(5)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(6)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(imageData->GetNumberOfCells()-1)<<std::endl;
    //std::cout<<"lalalal "<< temp->GetValue(imageData->GetNumberOfCells())<<std::endl;
    //(imageData->GetCellData()->GetScalars())->PrintSelf(std::cout,vtkIndent(0));//->GetData(0,10,0,0,temp);
    //temp->GetValue(0);
    //std::cout<<"TEMP:>VTK: "<<temp->GetValue(0)  <<std::endl;
    //std::cout<<"---->VTK: "<<temp[0]  <<std::endl;
    //std::cout<<"---->VTK: "<<*imageData->GetCell(1,1,0)  <<std::endl;
    //vtkCell* temp = imageData->GetCell(1,1,0);
    //std::cout<<"---->VTK: "<<pixel[0] << "\t"<<pixel[1] << "\t"<< pixel[2] <<std::endl;
}
//TODO: State vti_to_af(const af::array& field, const Mesh& mesh, std::string filename){
