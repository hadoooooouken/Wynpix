#include <QBuffer>
#include <QImage>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include "components/cache/decodedthumbnailcache.h"
#include "components/cache/thumbnailcache.h"
#include "testsuites.h"

namespace {
using namespace Qt::StringLiterals;

QByteArray encodePng(int width, int height, QColor color) {
  QImage image(width, height, QImage::Format_RGB32);
  image.fill(color);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return bytes;
}

ThumbnailCache::WriteEntry makeEntry(const QString &id,
                                     const ThumbnailSourceStamp &stamp,
                                     int width, int height,
                                     bool toneMapDependent = false,
                                     bool toneMapEnabled = false) {
  return {
      .id = id,
      .sourceStamp = stamp,
      .originalWidth = 4000,
      .originalHeight = 1000,
      .cachedWidth = width,
      .cachedHeight = height,
      .label = u"4000x1000"_s,
      .encodedData = encodePng(width, height, Qt::blue),
      .requiresLinearColorSpace = false,
      .toneMapDependent = toneMapDependent,
      .toneMapEnabled = toneMapEnabled,
      .toneMapOperator = 0,
      .toneMapWhiteLevel = 0,
  };
}
} // namespace

class ThumbnailCacheTests : public QObject {
  Q_OBJECT

private slots:
  void decodedCacheNeverDowngradesMatchingSource() {
    DecodedThumbnailCache cache(32 * 1024 * 1024);
    const ThumbnailSourceStamp stamp{u"c:/img/pano.jpg"_s, 100, 4096};
    const QString id = u"pano-id"_s;

    QImage smallImg(256, 64, QImage::Format_RGB32);
    QImage largeImg(1024, 256, QImage::Format_RGB32);
    QImage mediumImg(576, 144, QImage::Format_RGB32);

    cache.insert(id, stamp, smallImg, false, 1000, false, false, 0, 0);
    cache.insert(id, stamp, largeImg, false, 1001, false, false, 0, 0);
    auto lookup = cache.lookup(id, stamp, 1002, 3600);
    QVERIFY(lookup.image);
    QCOMPARE(lookup.image->size(), QSize(1024, 256));

    // Concurrent smaller entry for same stamp and tone-map must not downgrade it.
    cache.insert(id, stamp, mediumImg, false, 1003, false, false, 0, 0);
    lookup = cache.lookup(id, stamp, 1004, 3600);
    QVERIFY(lookup.image);
    QCOMPARE(lookup.image->size(), QSize(1024, 256));

    // Changed tone-map settings or newer source stamp may replace with smaller image.
    cache.insert(id, stamp, mediumImg, false, 1005, true, true, 1, 200);
    lookup = cache.lookup(id, stamp, 1006, 3600);
    QVERIFY(lookup.image);
    QCOMPARE(lookup.image->size(), QSize(576, 144));

    const ThumbnailSourceStamp newerStamp{u"c:/img/pano.jpg"_s, 101, 4096};
    cache.insert(id, newerStamp, smallImg, false, 1007, true, true, 1, 200);
    lookup = cache.lookup(id, newerStamp, 1008, 3600);
    QVERIFY(lookup.image);
    QCOMPARE(lookup.image->size(), QSize(256, 64));
  }

  void sqliteUpsertIsMonotonicAndCaseInsensitive() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dbPath = dir.filePath(u"thumbnails.db"_s);
    const QString id = u"pano-sql-id"_s;
    const ThumbnailSourceStamp lowerStamp{u"c:/img/pano.jpg"_s, 100, 4096};
    const ThumbnailSourceStamp upperStamp{u"C:/IMG/Pano.jpg"_s, 100, 4096};

    {
      ThumbnailCache writerCache(dbPath);
      QVERIFY(writerCache.saveThumbnails(
          {makeEntry(id, lowerStamp, 1024, 256)}));
      // Smaller write with different path case must succeed without downgrading.
      QVERIFY(writerCache.saveThumbnails(
          {makeEntry(id, upperStamp, 576, 144)}));
    }
    {
      ThumbnailCache readerCache(dbPath);
      const auto result = readerCache.readThumbnail(id, lowerStamp);
      QVERIFY(result.image);
      QCOMPARE(result.image->size(), QSize(1024, 256));
    }

    // Newer source stamp replaces even with a smaller thumbnail.
    const ThumbnailSourceStamp newerStamp{u"c:/img/pano.jpg"_s, 101, 4096};
    {
      ThumbnailCache writerCache(dbPath);
      QVERIFY(writerCache.saveThumbnails(
          {makeEntry(id, newerStamp, 256, 64)}));
    }
    {
      ThumbnailCache readerCache(dbPath);
      const auto result = readerCache.readThumbnail(id, newerStamp);
      QVERIFY(result.image);
      QCOMPARE(result.image->size(), QSize(256, 64));
    }

    // Changed tone-map settings replace even with a smaller thumbnail.
    {
      ThumbnailCache writerCache(dbPath);
      QVERIFY(writerCache.saveThumbnails(
          {makeEntry(id, newerStamp, 128, 32, true, true)}));
    }
    {
      ThumbnailCache readerCache(dbPath);
      const auto result = readerCache.readThumbnail(id, newerStamp);
      QVERIFY(result.image);
      QCOMPARE(result.image->size(), QSize(128, 32));
    }
  }

  void sqliteMigratesLegacySchemaAndUpgradesZeroSizedRows() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dbPath = dir.filePath(u"legacy.db"_s);
    const QString id = u"legacy-id"_s;
    const ThumbnailSourceStamp stamp{u"c:/img/legacy.jpg"_s, 50, 2048};
    const QString connName = u"legacy_schema_setup"_s;

    {
      QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connName);
      db.setDatabaseName(dbPath);
      QVERIFY(db.open());
      QSqlQuery q(db);
      QVERIFY(q.exec(QStringLiteral(
          "CREATE TABLE thumbnails ("
          "id TEXT PRIMARY KEY, "
          "last_modified TEXT, "
          "original_width INTEGER, "
          "original_height INTEGER, "
          "label TEXT, "
          "data BLOB, "
          "source_path TEXT, "
          "source_mtime INTEGER, "
          "source_size INTEGER, "
          "requires_linear_color_space INTEGER NOT NULL DEFAULT 0, "
          "tone_map_dependent INTEGER, "
          "tone_map_enabled INTEGER NOT NULL DEFAULT 0, "
          "tone_map_operator INTEGER NOT NULL DEFAULT 0, "
          "tone_map_white_level INTEGER NOT NULL DEFAULT 0, "
          "last_accessed INTEGER NOT NULL DEFAULT 0);")));
      QVERIFY(q.prepare(QStringLiteral(
          "INSERT INTO thumbnails (id, last_modified, original_width, original_height, "
          "label, data, source_path, source_mtime, source_size, last_accessed) "
          "VALUES (:id, '50', 4000, 1000, '4000x1000', :data, :path, 50, 2048, 100);")));
      q.bindValue(u":id"_s, id);
      q.bindValue(u":data"_s, encodePng(256, 64, Qt::red));
      q.bindValue(u":path"_s, stamp.normalizedPath);
      QVERIFY(q.exec());
      db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    {
      ThumbnailCache cache(dbPath);
      const auto legacyRead = cache.readThumbnail(id, stamp);
      QVERIFY(legacyRead.image);
      QCOMPARE(legacyRead.image->size(), QSize(256, 64));

      // Migrated row has cached_width=0, so larger write upgrades it in-place.
      QVERIFY(cache.saveThumbnails({makeEntry(id, stamp, 1024, 256)}));
    }
    {
      ThumbnailCache verifyCache(dbPath);
      const auto upgradedRead = verifyCache.readThumbnail(id, stamp);
      QVERIFY(upgradedRead.image);
      QCOMPARE(upgradedRead.image->size(), QSize(1024, 256));
    }
  }
};

int runThumbnailCacheTests(int argc, char **argv) {
  ThumbnailCacheTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_thumbnailcache.moc"
