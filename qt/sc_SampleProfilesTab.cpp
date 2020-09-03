#include "sc_SampleProfilesTab.hpp"
#include <QDir>
#include "simulationcraftqt.hpp"

SC_SampleProfilesTab::SC_SampleProfilesTab( QWidget* parent ) :
    QGroupBox( parent ),
    tree( new QTreeWidget( this ) )
{
    QStringList headerLabels( tr( "Player Class" ) ); headerLabels += QString( tr( "Location" ) );
    tree -> setColumnCount( 1 );
    tree -> setHeaderLabels( headerLabels );
    tree -> setColumnWidth( 0, 300 );

    // Create BiS Introduction
    QFormLayout* bisIntroductionFormLayout = new QFormLayout();
    QLabel* bisText = new QLabel( tr( "These sample profiles are attempts at creating the best possible gear, talent, and action priority list setups to achieve the highest possible average damage per second.\n"
      "The profiles are created with a lot of help from the theorycrafting community.\n"
      "They are only as good as the thorough testing done on them, and the feedback and critic we receive from the community, including yourself.\n"
      "If you have ideas for improvements, try to simulate them. If they result in increased dps, please open a ticket on our Issue tracker.\n"
      "The more people help improve BiS profiles, the better will they reach their goal of representing the highest possible dps." ) );
    bisIntroductionFormLayout -> addRow( bisText );
    QWidget* bisIntroduction = new QWidget();
    bisIntroduction -> setLayout( bisIntroductionFormLayout );

    for(const auto& path : SC_PATHS::getDataPaths())
    {
      QDir baseDir(path + "/profiles");
      qDebug() << "Sample Profiles Base Path: " << baseDir.absolutePath() << " exists: " << baseDir.exists();
      if ( baseDir.exists() )
      {
         fillTree( baseDir );
      }
    }


    QVBoxLayout* bisTabLayout = new QVBoxLayout();
    bisTabLayout -> addWidget( bisIntroduction, 1 );
    bisTabLayout -> addWidget( tree, 9 );

    setLayout( bisTabLayout );
}

void SC_SampleProfilesTab::fillTree( QDir baseDir )
{
    baseDir.setFilter( QDir::Dirs );

    static const char* tierNames[] = { "T25", "PR", "DS" };
    static const int TIER_MAX = 6; // = range::size( tierNames );

    QTreeWidgetItem* playerItems[PLAYER_MAX];
    range::fill( playerItems, nullptr );
    std::array<std::array<QTreeWidgetItem*, TIER_MAX>,PLAYER_MAX> rootItems;
    if ( rootItems.size() )
    {
      for ( size_t i = 0u; i < rootItems.size(); ++i )
      {
        range::fill( rootItems[i], nullptr );
      }
    }


    QStringList tprofileList = baseDir.entryList();
    int tnumProfiles = tprofileList.count();
    // Main loop through all subfolders of ./profiles/
    for ( int i = 0; i < tnumProfiles; i++ )
    {
      QDir dir = baseDir.absolutePath() + "/" + tprofileList[ i ];
      dir.setSorting( QDir::Name );
      dir.setFilter( QDir::Files );
      dir.setNameFilters( QStringList( "*.simc" ) );

      QStringList profileList = dir.entryList();
      int numProfiles = profileList.count();
      for ( int k = 0; k < numProfiles; k++ )
      {
        QString profile = dir.absolutePath() + "/";
        profile = QDir::toNativeSeparators( profile );
        profile += profileList[k];

        player_e player = PLAYER_MAX;

        // Hack! For now...  Need to decide sim-wide just how the heck we want to refer to DKs.
        // Hack²! For now... I didn't know how to add DH :D
        if ( profile.contains( "Death_Knight" ) )
          player = DEATH_KNIGHT;
        else if ( profile.contains( "Demon_Hunter" ) )
          player = DEMON_HUNTER;
        else
        {
          for ( player_e j = PLAYER_NONE; j < PLAYER_MAX; j++ )
          {
            if ( profile.contains( util::player_type_string( j ), Qt::CaseInsensitive ) )
            {
              player = j;
              break;
            }
          }
        }

        // exclude generate profiles
        if ( profile.contains( "generate" ) )
          continue;

        int tier = TIER_MAX;
        for ( int j = 0; j < TIER_MAX && tier == TIER_MAX; j++ )
          if ( profile.contains( tierNames[j] ) )
            tier = j;

        if ( player != PLAYER_MAX && tier != TIER_MAX )
        {
          if ( !rootItems[player][tier] )
          {
            if ( !playerItems[player] )
            {
              QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util::inverse_tokenize( util::player_type_string( player ) ).c_str() ) );
              playerItems[player] = top;
            }

            if ( !rootItems[player][tier] )
            {
              QTreeWidgetItem* tieritem = new QTreeWidgetItem( QStringList( tierNames[tier] ) );
              playerItems[player] -> addChild( rootItems[player][tier] = tieritem );
            }
          }

          QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << profileList[k] << profile );
      assert(rootItems[player][tier]);
      assert(item);
          rootItems[player][tier] -> addChild( item );
        }
      }
    }

    // Register all the added profiles ( done here so they show up alphabetically )
    for ( player_e i = DEATH_KNIGHT; i <= WARRIOR; i++ )
    {
      if ( playerItems[i] )
      {
        tree -> addTopLevelItem( playerItems[i] );
        for ( int j = 0; j < TIER_MAX; j++ )
        {
          if ( rootItems[i][j] )
          {
            rootItems[i][j] -> setExpanded( true ); // Expand the subclass Tier bullets by default
            rootItems[i][j] -> sortChildren( 0, Qt::AscendingOrder );
          }
        }
      }
    }

}
